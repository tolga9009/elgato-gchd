add_library(gchd_core STATIC
  disk.cpp
  fifo.cpp
  firmware.cpp
  gchd.cpp
  process.cpp
  output_io.cpp
  output_queue.cpp
  socket.cpp
  streamer.cpp
  utility.cpp
  gchd/commands.cpp
  gchd/configure.cpp
  gchd/configure_component.cpp
  gchd/configure_composite.cpp
  gchd/configure_hdmi.cpp
  gchd/psi_data.cpp
  gchd/psi_descriptors.cpp
  gchd/psi_pat.cpp
  gchd/psi_pmt.cpp
  gchd/psi_sit.cpp
  gchd/psi_syntax.cpp
  gchd/psi_table_header.cpp
  gchd/settings.cpp
  gchd/transcoder.cpp
)
target_include_directories(gchd_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(gchd_core PUBLIC gchd_options PkgConfig::LIBUSB Threads::Threads OpenSSL::Crypto)
add_executable(gchd main.cpp)
target_link_libraries(gchd PRIVATE gchd_core)
install(TARGETS gchd RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
