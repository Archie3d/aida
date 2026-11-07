include(${CMAKE_CURRENT_LIST_DIR}/ProcessLimits.cmake)
foreach(family Text_IO Wide_Text_IO Wide_Wide_Text_IO)
    set(character Character)
    if(family STREQUAL Wide_Text_IO)
        set(character Wide_Character)
    elseif(family STREQUAL Wide_Wide_Text_IO)
        set(character Wide_Wide_Character)
    endif()
    set(work "${WORKDIR}/${family}")
    file(MAKE_DIRECTORY "${work}")
    set(program "${work}/standard_stream${CMAKE_EXECUTABLE_SUFFIX}")
    set(source "${work}/standard_stream.adb")
    file(WRITE "${source}" "with Ada.${family};
with Ada.${family}.Text_Streams;
procedure Standard_Stream is
    package T renames Ada.${family};
    package S renames Ada.${family}.Text_Streams;
    use type T.Count;
    C : ${character};
    B : Character;
    E : Boolean;
begin
    T.Look_Ahead (T.Standard_Input, C, E);
    if C /= ${character}'Val (65) or E then raise Program_Error; end if;
    Character'Read (S.Stream (T.Current_Input), B);
    if B /= 'A' or T.Col (T.Current_Input) /= 1 then raise Program_Error; end if;
    T.Get (C);
    if C /= ${character}'Val (66) or T.Col (T.Current_Input) /= 2 or not T.End_Of_File then raise Program_Error; end if;
    Character'Write (S.Stream (T.Standard_Output), 'R');
    if T.Col /= 1 then raise Program_Error; end if;
    T.Put (${character}'Val (84));
    Character'Write (S.Stream (T.Current_Error), 'E');
end Standard_Stream;")
    execute_process(COMMAND "${ADA}" -o "${program}" "${source}"
        RESULT_VARIABLE status OUTPUT_VARIABLE output ERROR_VARIABLE errors TIMEOUT ${AIDA_COMPILE_TIMEOUT})
    if(NOT status STREQUAL "0")
        message(FATAL_ERROR "Standard-stream build failed: ${output}${errors}")
    endif()
    file(WRITE "${work}/input" "AB")
    foreach(mode file pipe)
        if(mode STREQUAL pipe)
            execute_process(COMMAND "${CMAKE_COMMAND}" -E cat "${work}/input" COMMAND "${program}"
                RESULT_VARIABLE status OUTPUT_VARIABLE output ERROR_VARIABLE errors TIMEOUT ${AIDA_RUN_TIMEOUT})
        else()
            execute_process(COMMAND "${program}" INPUT_FILE "${work}/input"
                RESULT_VARIABLE status OUTPUT_VARIABLE output ERROR_VARIABLE errors TIMEOUT ${AIDA_RUN_TIMEOUT})
        endif()
        if(NOT status STREQUAL "0" OR NOT output STREQUAL "RT" OR NOT errors STREQUAL "E")
            message(FATAL_ERROR "${family} ${mode} standard streams failed (${status}): ${output}${errors}")
        endif()
    endforeach()
endforeach()

# File_Type and Stream_Access are distinct for each parent family.
set(source "${CMAKE_CURRENT_LIST_DIR}/ada/io/textstreamerrors.adb")
execute_process(COMMAND "${ADA}" -o "${WORKDIR}/invalid" "${source}"
    RESULT_VARIABLE status OUTPUT_VARIABLE output ERROR_VARIABLE errors TIMEOUT ${AIDA_COMPILE_TIMEOUT})
string(REPLACE "${source}" "textstreamerrors.adb" errors "${errors}")
file(READ "${CMAKE_CURRENT_LIST_DIR}/ada/io/textstreamerrors.expected" expected_errors)
if(NOT status STREQUAL "1" OR NOT output STREQUAL "" OR NOT errors STREQUAL expected_errors)
    message(FATAL_ERROR "Incorrect text-stream family diagnostics (${status}): ${output}${errors}")
endif()
