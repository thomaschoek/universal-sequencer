project('nlohmann_json', 'cpp',
  version: '3.11.3',
  license: 'MIT'
)

# Header-only library
nlohmann_json_inc = include_directories('include')

nlohmann_json_dep = declare_dependency(
  include_directories: nlohmann_json_inc
)

meson.override_dependency('nlohmann_json', nlohmann_json_dep)
