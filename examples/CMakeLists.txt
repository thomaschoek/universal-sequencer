# Example programs for MicroComposer
add_executable(synth_example
    synth_example.cpp
    ${CMAKE_SOURCE_DIR}/src/sequencer/step_sequencer.cpp
    ${CMAKE_SOURCE_DIR}/src/sequence/atomic_step_sequence.cpp
    ${CMAKE_SOURCE_DIR}/src/synth/synth.cpp
    ${CMAKE_SOURCE_DIR}/src/sequencer/step_sequencer_output.cpp
    ${CMAKE_SOURCE_DIR}/src/sequencer/synth_sequencer.cpp
)

add_executable(simple_audio_example
    simple_audio_example.cpp
    ${CMAKE_SOURCE_DIR}/src/sequencer/step_sequencer.cpp
    ${CMAKE_SOURCE_DIR}/src/sequence/atomic_step_sequence.cpp
    ${CMAKE_SOURCE_DIR}/src/synth/synth.cpp
    ${CMAKE_SOURCE_DIR}/src/sequencer/step_sequencer_output.cpp
    ${CMAKE_SOURCE_DIR}/src/sequencer/synth_sequencer.cpp
)

target_include_directories(synth_example PRIVATE ${CMAKE_SOURCE_DIR}/include)
target_include_directories(simple_audio_example PRIVATE ${CMAKE_SOURCE_DIR}/include)