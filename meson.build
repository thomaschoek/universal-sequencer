project('MicroComposer', 'cpp',
  version: '1.0.0',
  default_options: [
    'cpp_std=c++20',
    'warning_level=2',
    'buildtype=debug',
  ]
)

# Include directories
inc = include_directories('include')

# System dependencies with pkg-config
gtkmm4_dep = dependency('gtkmm-4.0',
  required: false,
  fallback: ['gtkmm-4.0', 'gtkmm_dep']
)
alsa_dep = dependency('alsa', required: false)
threads_dep = dependency('threads')

# External dependencies (try system first, fallback to subprojects)
# readerwriterqueue - header-only lock-free queue library
readerwriterqueue_dep = dependency('readerwriterqueue',
  required: false,
  fallback: ['readerwriterqueue', 'readerwriterqueue_dep']
)

# libremidi - MIDI library
libremidi_dep = dependency('libremidi',
  version: '>=5.3.0',
  required: false,
  fallback: ['libremidi', 'libremidi_dep']
)

# nlohmann/json - JSON library for C++
nlohmann_json_dep = dependency('nlohmann_json',
  version: '>=3.11.0',
  required: false,
  fallback: ['nlohmann_json', 'nlohmann_json_dep']
)

# Catch2 - testing framework (only needed for tests)
# Will use WrapDB if not found on system
catch2_dep = dependency('catch2-with-main',
  version: '>=3.0.0',
  required: false,
  fallback: ['catch2', 'catch2_with_main_dep']
)

# Copy resources to build directory
# Use a simple script to copy the CSS file
run_command('mkdir', '-p', meson.current_build_dir() / 'resources', check: true)
configure_file(
  input: 'resources/gui_style.css',
  output: '@BASENAME@',
  copy: true
)

# Also copy to resources subdirectory for GUI runtime access
custom_target('copy_css_to_resources',
  input: 'resources/gui_style.css',
  output: 'css_copied',
  command: ['cp', '@INPUT@', meson.current_build_dir() / 'resources' / 'gui_style.css'],
  build_by_default: true,
  console: true
)

# Common source files
synth_output_src = files('src/synth/synth_output.cpp')
midi_output_src = files(
  'src/midi/midi_output.cpp',
  'src/sequencable/midi_event.cpp'
)

# Executable 1: MicroComposerCLI (Console Application)
if gtkmm4_dep.found()
  executable('MicroComposerCLI',
    sources: [
      'src/main.cpp',
      synth_output_src
    ],
    include_directories: inc,
    dependencies: [gtkmm4_dep],
    install: true
  )
endif

# Executable 2: MicroComposer (GUI Application)
if gtkmm4_dep.found() and nlohmann_json_dep.found()
  executable('MicroComposer',
    sources: [
      'src/main_gui.cpp',
      synth_output_src
    ],
    include_directories: inc,
    dependencies: [gtkmm4_dep, nlohmann_json_dep],
    install: true
  )
endif

# Executable 3: MicroComposerMIDI (MIDI Console Application)
if libremidi_dep.found() and alsa_dep.found()
  executable('MicroComposerMIDI',
    sources: [
      'src/main_midi.cpp',
      midi_output_src
    ],
    include_directories: inc,
    dependencies: [
      libremidi_dep,
      alsa_dep,
      threads_dep
    ],
    install: true
  )
endif

# Executable 4: midiguy (MIDI GUI Application)
if gtkmm4_dep.found() and libremidi_dep.found() and alsa_dep.found() and nlohmann_json_dep.found()
  executable('midiguy',
    sources: [
      'src/main_midi_gui.cpp',
      midi_output_src
    ],
    include_directories: inc,
    dependencies: [
      gtkmm4_dep,
      libremidi_dep,
      alsa_dep,
      threads_dep,
      nlohmann_json_dep
    ],
    install: true
  )
endif

# Tests subdirectory
if catch2_dep.found()
  subdir('tests')
endif
