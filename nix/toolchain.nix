# Assembly workflow: Gruntz nix/toolchain.nix at 7d4bd55b99e32f084834d991badf7609889481f4.
# Compiler media: HoMM1 Buka config/toolchains.json at d3c9297135800ee0d198b292f2424464d20b6f5c.
{ pkgs, sp5Xtree ? ../private/vc6-sp5/XTREE }:
let
  base = pkgs.fetchurl {
    url = "https://github.com/sushi-shi/homm1-decomp/releases/download/toolchain-buka-2003-v2/homm1-toolchain-buka-2003-v2.tar.xz";
    sha256 = "ea8efe04f52f36e0371dd5dde77e25e552a2f221819a663ca39530b005b3713c";
  };
  processorPack = pkgs.fetchurl {
    url = "https://download.microsoft.com/download/vb60ent/Update/6/W9X2KXP/EN-US/vcpp5.exe";
    sha256 = "96817df1bb0cb58e5b1d0fbcc5b993a63548182add1fbff66365401203ecfb6c";
  };
in pkgs.runCommand "hobbit-toolchain-vc6" {
  nativeBuildInputs = [ pkgs.gnutar pkgs.xz pkgs.p7zip ];
} ''
  mkdir -p "$out" unpack
  tar xf ${base} -C unpack
  cp -r unpack/toolchains/vc6 "$out/msvc"
  chmod -R u+w "$out/msvc"
  7z x -y -oprocessor-pack ${processorPack} > /dev/null
  # SP5 Processor Pack C2 is 12.00.9044 (0x2354), Hobbit's dominant Rich build.
  cp processor-pack/c2.dll "$out/msvc/bin/C2.DLL"
  cp processor-pack/ml.exe "$out/msvc/bin/ML.EXE"
  cp processor-pack/ml.err "$out/msvc/bin/ML.ERR"
  # Complete SP5 XTREE extracted from pinned local media by the HoMM1-derived
  # installer. Proprietary payload is staged only in ignored shell snapshots.
  test "$(sha256sum ${sp5Xtree} | cut -d ' ' -f 1)" = "d0280d2cb8c2b2d14ce60dc868da5159adf1f5844a971d623883ffba4da35354"
  cp ${sp5Xtree} "$out/msvc/include/XTREE"
  # Exact [CopyToVC98Include] payload/renames from Microsoft's vcpp.inf.
  for header in dvec.h fpieee.h fvec.h ivec.h malloc.h mm3dnow.h mmintrin.h; do
    cp "processor-pack/$header" "$out/msvc/include/$header"
  done
  cp processor-pack/emmint_1.h "$out/msvc/include/emmintrin.h"
  cp processor-pack/xmmint_1.h "$out/msvc/include/xmmintrin.h"
''
