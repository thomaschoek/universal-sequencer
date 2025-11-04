# Test sources
test_sources = files(
  'main.cpp',
  'test_clock.cpp',
  'test_timing_capabilities.cpp',
  'sequencer/test_sequencer_template.cpp',
  'sequencer/test_poly_sequencer.cpp',
  'controller/test_poly_sequencer_controller.cpp',
  'gui/test_json_serialization.cpp',

  # Disabled tests (compilation issues):
  # 'container/test_atomic_deque.cpp',
  # 'container/test_atomic_ring_deque.cpp',
  # 'container/test_atomic_vector.cpp',
)

# Build test executable
test_exe = executable('tests',
  sources: [
    test_sources,
    synth_output_src  # From parent meson.build
  ],
  include_directories: inc,  # From parent meson.build
  dependencies: [catch2_dep],
  build_by_default: true
)

# Register tests with Meson
test('MicroComposer Tests',
  test_exe,
  args: ['--reporter', 'compact'],
  timeout: 60
)
