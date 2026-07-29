# Maintaining

Before release, run shared and static Meson matrices, strict compiler builds,
ASan/UBSan, contract scripts, `git diff --check`, and `git fsck`. Inspect the
shared object's SONAME and `DT_NEEDED` closure. ABI changes require an explicit
SONAME decision.
