# Exercise grading failures with real compiler/program invocations. In
# particular, an exit code of zero alone must not imply a conformance pass.
foreach(scenario failed missing_result wrong_rejection changed_rejection unexpected_pass bad_output_expectation)
    set(root "${WORKDIR}/${scenario}")
    file(MAKE_DIRECTORY "${root}/upstream")
    file(COPY "${ROOT}/support" DESTINATION "${root}")
    set(rejection "")
    set(expected_file "")
    set(expected "ACATS c53007a failed")
    if(scenario STREQUAL "failed")
        set(body "Failed (\"deliberate failure\"); Result;")
    elseif(scenario STREQUAL "missing_result")
        set(body "null;")
    elseif(scenario STREQUAL "bad_output_expectation")
        set(body "null;")
        set(expected_file "incomplete.expected")
        file(WRITE "${root}/${expected_file}" "TEST C53007A\n")
        set(expected "Expected ACATS output must retain the successful REPORT protocol")
    elseif(scenario STREQUAL "unexpected_pass")
        set(body "Result;")
        set(rejection "goto target 'Finish' does not denote a label")
        set(expected "XPASS c53007a")
    else()
        set(body "goto Finish;")
        set(rejection "unrelated diagnostic")
        set(expected "Unexpected compilation failure")
        if(scenario STREQUAL "changed_rejection")
            set(rejection "goto target 'Finish' does not denote a label")
            file(WRITE "${root}/c53007a.diagnostics" "different diagnostic\n")
            set(expected "Changed XFAIL diagnostics")
        endif()
    endif()
    file(WRITE "${root}/upstream/c53007a.ada"
        "with Report; use Report;\nprocedure C53007A is\nbegin\n    Test (\"C53007A\", \"Harness self-test\");\n    ${body}\nend C53007A;\n")
    execute_process(
        COMMAND "${CMAKE_COMMAND}" "-DADA=${ADA}" -DNAME=c53007a
            "-DROOT=${root}" "-DWORKDIR=${root}/work"
            "-DEXPECTED_OUTPUT_FILE=${expected_file}"
            "-DREJECTION=${rejection}" -P "${ROOT}/RunAcats.cmake"
        TIMEOUT 55
        RESULT_VARIABLE status OUTPUT_VARIABLE output ERROR_VARIABLE errors
    )
    string(FIND "${errors}" "${expected}" found)
    if(NOT status STREQUAL "1" OR found EQUAL -1)
        message(FATAL_ERROR "Harness incorrectly graded ${scenario} (${status}):\n${output}${errors}")
    endif()
endforeach()

# C390012 accepts six elaboration orders. Verify both alternative matching and
# rejection, and that successful validation clears the accumulated trace.
foreach(scenario alternatives bad_alternative)
    set(root "${WORKDIR}/${scenario}")
    file(MAKE_DIRECTORY "${root}/upstream")
    file(COPY "${ROOT}/support" DESTINATION "${root}")
    if(scenario STREQUAL "alternatives")
        set(body "for C in Character range 'a' .. 'f' loop
        TCTouch.Touch (C);
        TCTouch.Validate_One_Of (\"a\", \"b\", \"c\", \"d\", \"e\", \"f\", \"alternative mismatch\");
    end loop;
    TCTouch.Validate (\"\", \"trace was not cleared\");")
    else()
        set(body "TCTouch.Touch ('z');
    TCTouch.Validate_One_Of (\"a\", \"b\", \"c\", \"d\", \"e\", \"f\", \"deliberate mismatch\");")
    endif()
    file(WRITE "${root}/upstream/c390012.a"
        "with Report; use Report; with TCTouch;
procedure C390012 is
begin
    Test (\"C390012\", \"Harness self-test\");
    ${body}
    Result;
end C390012;")
    execute_process(
        COMMAND "${CMAKE_COMMAND}" "-DADA=${ADA}" -DNAME=c390012 -DSOURCE_LIST=c390012.a
            "-DROOT=${root}" "-DWORKDIR=${root}/work" -P "${ROOT}/RunAcats.cmake"
        TIMEOUT 55
        RESULT_VARIABLE status OUTPUT_VARIABLE output ERROR_VARIABLE errors
    )
    if(scenario STREQUAL "alternatives")
        if(NOT status STREQUAL "0")
            message(FATAL_ERROR "Harness rejected valid alternatives:\n${output}${errors}")
        endif()
    else()
        string(FIND "${errors}" "ACATS c390012 failed" found)
        if(NOT status STREQUAL "1" OR found EQUAL -1)
            message(FATAL_ERROR "Harness accepted an invalid alternative:\n${output}${errors}")
        endif()
    endif()
endforeach()
