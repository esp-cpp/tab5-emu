# Star Wars Episode I: The Phantom Menace (PSX), natively: the tpm-decomp
# decompilation (https://github.com/esp-cpp/tpm-decomp, submodule tpm-decomp/) on
# psyz, as components/mgs does for Metal Gear Solid. The game is not
# decompiled yet: CONFIG_TPM_ENGINE is off by default and the component then
# builds only the platform half (src/), which reads the disc files and shows
# the load screens.

set(TPM_SRCS "src/tpm.cpp" "src/tpm_files.cpp")

idf_component_register(
  SRCS ${TPM_SRCS}
  INCLUDE_DIRS "include"
  PRIV_INCLUDE_DIRS "src"
  REQUIRES tab5-emu
  PRIV_REQUIRES esp_timer
)
