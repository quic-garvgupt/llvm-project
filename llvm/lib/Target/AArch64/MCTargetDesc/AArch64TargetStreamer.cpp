//===- AArch64TargetStreamer.cpp - AArch64TargetStreamer class ------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// This file implements the AArch64TargetStreamer class.
//
//===----------------------------------------------------------------------===//

#include "AArch64TargetStreamer.h"
#include "AArch64MCAsmInfo.h"
#include "llvm/BinaryFormat/ELF.h"
#include "llvm/MC/ConstantPools.h"
#include "llvm/MC/MCContext.h"
#include "llvm/MC/MCELFStreamer.h"
#include "llvm/MC/MCSection.h"
#include "llvm/MC/MCSectionELF.h"
#include "llvm/MC/MCSubtargetInfo.h"
#include "llvm/Support/CommandLine.h"

using namespace llvm;

static cl::opt<bool> MarkBTIProperty(
    "aarch64-mark-bti-property", cl::Hidden,
    cl::desc("Add .note.gnu.property with BTI to assembly files"),
    cl::init(false));

//
// AArch64TargetStreamer Implementation
//
AArch64TargetStreamer::AArch64TargetStreamer(MCStreamer &S)
    : MCTargetStreamer(S), ConstantPools(new AssemblerConstantPools()) {}

AArch64TargetStreamer::~AArch64TargetStreamer() = default;

void AArch64TargetStreamer::emitAuthValue(const MCExpr *Expr,
                                          uint16_t Discriminator,
                                          AArch64PACKey::ID Key,
                                          bool HasAddressDiversity) {
  Streamer.emitValueImpl(AArch64AuthMCExpr::create(Expr, Discriminator, Key,
                                                   HasAddressDiversity,
                                                   Streamer.getContext()),
                         8);
}

// The constant pool handling is shared by all AArch64TargetStreamer
// implementations.
const MCExpr *AArch64TargetStreamer::addConstantPoolEntry(const MCExpr *Expr,
                                                          unsigned Size,
                                                          SMLoc Loc) {
  return ConstantPools->addEntry(Streamer, Expr, Size, Loc);
}

void AArch64TargetStreamer::emitCurrentConstantPool() {
  ConstantPools->emitForCurrentSection(Streamer);
}

void AArch64TargetStreamer::emitConstantPools() {
  ConstantPools->emitAll(Streamer);
}

// finish() - write out any non-empty assembler constant pools and
//   write out note.gnu.properties if need.
void AArch64TargetStreamer::finish() {
  if (MarkBTIProperty)
    emitNoteSection(ELF::GNU_PROPERTY_AARCH64_FEATURE_1_BTI);
}

void AArch64TargetStreamer::emitNoteSection(unsigned Flags,
                                            uint64_t PAuthABIPlatform,
                                            uint64_t PAuthABIVersion) {
  assert((PAuthABIPlatform == uint64_t(-1)) ==
         (PAuthABIVersion == uint64_t(-1)));
  uint64_t DescSz = 0;
  if (Flags != 0)
    DescSz += 4 * 4;
  if (PAuthABIPlatform != uint64_t(-1))
    DescSz += 4 + 4 + 8 * 2;
  if (DescSz == 0)
    return;

  MCStreamer &OutStreamer = getStreamer();
  MCContext &Context = OutStreamer.getContext();
  // Emit a .note.gnu.property section with the flags.
  MCSectionELF *Nt = Context.getELFSection(".note.gnu.property", ELF::SHT_NOTE,
                                           ELF::SHF_ALLOC);
  if (Nt->isRegistered()) {
    SMLoc Loc;
    Context.reportWarning(
        Loc,
        "The .note.gnu.property is not emitted because it is already present.");
    return;
  }
  MCSection *Cur = OutStreamer.getCurrentSectionOnly();
  OutStreamer.switchSection(Nt);

  // Emit the note header.
  OutStreamer.emitValueToAlignment(Align(8));
  OutStreamer.emitIntValue(4, 4);     // data size for "GNU\0"
  OutStreamer.emitIntValue(DescSz, 4); // Elf_Prop array size
  OutStreamer.emitIntValue(ELF::NT_GNU_PROPERTY_TYPE_0, 4);
  OutStreamer.emitBytes(StringRef("GNU", 4)); // note name

  // Emit the PAC/BTI properties.
  if (Flags != 0) {
    OutStreamer.emitIntValue(ELF::GNU_PROPERTY_AARCH64_FEATURE_1_AND, 4);
    OutStreamer.emitIntValue(4, 4);     // data size
    OutStreamer.emitIntValue(Flags, 4); // data
    OutStreamer.emitIntValue(0, 4);     // pad
  }

  // Emit the PAuth ABI compatibility info
  if (PAuthABIPlatform != uint64_t(-1)) {
    OutStreamer.emitIntValue(ELF::GNU_PROPERTY_AARCH64_FEATURE_PAUTH, 4);
    OutStreamer.emitIntValue(8 * 2, 4); // data size
    OutStreamer.emitIntValue(PAuthABIPlatform, 8);
    OutStreamer.emitIntValue(PAuthABIVersion, 8);
  }

  OutStreamer.endSection(Nt);
  OutStreamer.switchSection(Cur);
}

void AArch64TargetStreamer::emitInst(uint32_t Inst) {
  char Buffer[4];

  // We can't just use EmitIntValue here, as that will swap the
  // endianness on big-endian systems (instructions are always
  // little-endian).
  for (char &C : Buffer) {
    C = uint8_t(Inst);
    Inst >>= 8;
  }

  getStreamer().emitBytes(StringRef(Buffer, 4));
}

MCTargetStreamer *
llvm::createAArch64ObjectTargetStreamer(MCStreamer &S,
                                        const MCSubtargetInfo &STI) {
  const Triple &TT = STI.getTargetTriple();
  if (TT.isOSBinFormatELF())
    return new AArch64TargetELFStreamer(S);
  if (TT.isOSBinFormatCOFF())
    return new AArch64TargetWinCOFFStreamer(S);
  return nullptr;
}

MCTargetStreamer *llvm::createAArch64NullTargetStreamer(MCStreamer &S) {
  return new AArch64TargetStreamer(S);
}

void AArch64TargetStreamer::emitAttributesSubsection(
    StringRef VendorName, AArch64BuildAttributes::SubsectionOptional IsOptional,
    AArch64BuildAttributes::SubsectionType ParameterType) {

  // If exists, return.
  for (MCELFStreamer::AttributeSubSection &SubSection : AttributeSubSections) {
    if (VendorName == SubSection.VendorName) {
      activateAttributesSubsection(VendorName);
      return;
    }
  }
  // else, add the subsection
  MCELFStreamer::AttributeSubSection AttSubSection;
  AttSubSection.VendorName = VendorName;
  AttSubSection.IsOptional = IsOptional;
  AttSubSection.ParameterType = ParameterType;
  AttributeSubSections.push_back(AttSubSection);
  activateAttributesSubsection(VendorName);
}

std::unique_ptr<MCELFStreamer::AttributeSubSection>
AArch64TargetStreamer::getActiveAttributesSubsection() {
  for (MCELFStreamer::AttributeSubSection &SubSection : AttributeSubSections) {
    if (SubSection.IsActive) {
      return std::make_unique<MCELFStreamer::AttributeSubSection>(SubSection);
    }
  }
  return nullptr;
}

std::unique_ptr<MCELFStreamer::AttributeSubSection>
AArch64TargetStreamer::getAttributesSubsectionByName(StringRef Name) {
  for (MCELFStreamer::AttributeSubSection &SubSection : AttributeSubSections) {
    if (Name == SubSection.VendorName) {
      return std::make_unique<MCELFStreamer::AttributeSubSection>(SubSection);
    }
  }
  return nullptr;
}

void AArch64TargetStreamer::emitAttribute(StringRef VendorName, unsigned Tag,
                                          unsigned Value, std::string String) {

  if (unsigned(-1) == Value && "" == String) {
    assert(0 && "Arguments error");
    return;
  }
  if (AttributeSubSections.size() == 0) {
    assert(0 &&
           "Can not add AArch64 build attribute: no AArch64 subsection exists");
    return;
  }

  for (MCELFStreamer::AttributeSubSection &SubSection : AttributeSubSections) {
    if (VendorName == SubSection.VendorName) {
      if (!SubSection.IsActive) {
        assert(0 &&
               "Can not add AArch64 build attribute: subsection is not active");
        return;
      }
      for (MCELFStreamer::AttributeItem &Item : SubSection.Content) {
        // Tag already exists
        if (Item.Tag == Tag) {
          Item.Type = unsigned(-1) != Value
                          ? MCELFStreamer::AttributeItem::NumericAttribute
                          : MCELFStreamer::AttributeItem::TextAttribute;
          Item.IntValue = unsigned(-1) != Value ? Value : unsigned(-1);
          Item.StringValue = unsigned(-1) != Value ? "" : String;
          return;
        }
      }
      if (unsigned(-1) != Value)
        SubSection.Content.push_back(MCELFStreamer::AttributeItem(
            MCELFStreamer::AttributeItem::NumericAttribute, Tag, Value, ""));
      if ("" != String)
        SubSection.Content.push_back(MCELFStreamer::AttributeItem(
            MCELFStreamer::AttributeItem::TextAttribute, Tag, unsigned(-1),
            String));
      return;
    }
  }
  assert(0 && "Can not add AArch64 build attribute: required subsection does "
              "not exist");
}

void AArch64TargetStreamer::activateAttributesSubsection(StringRef VendorName) {
  for (MCELFStreamer::AttributeSubSection &SubSection : AttributeSubSections) {
    if (VendorName == SubSection.VendorName) {
      SubSection.IsActive = true;
    } else {
      SubSection.IsActive = false;
    }
  }
}
