# Exercise grading failures with real compiler/program invocations. In
# particular, an exit code of zero alone must not imply a conformance pass.
foreach(scenario failed missing_result wrong_rejection changed_rejection unexpected_pass)
    set(root "${WORKDIR}/${scenario}")
    file(MAKE_DIRECTORY "${root}/upstream")
    file(COPY "${ROOT}/support" DESTINATION "${root}")
    set(rejection "")
    set(expected "ACATS c53007a failed")
    if(scenario STREQUAL "failed")
        set(body "Failed (\"deliberate failure\"); Result;")
    elseif(scenario STREQUAL "missing_result")
        set(body "null;")
    elseif(scenario STREQUAL "unexpected_pass")
        set(body "Result;")
        set(rejection "goto statements are not yet supported")
        set(expected "XPASS c53007a")
    else()
        set(body "goto Finish;")
        set(rejection "unrelated diagnostic")
        set(expected "Unexpected compilation failure")
        if(scenario STREQUAL "changed_rejection")
            set(rejection "goto statements are not yet supported")
            file(WRITE "${root}/c53007a.diagnostics" "different diagnostic\n")
            set(expected "Changed XFAIL diagnostics")
        endif()
    endif()
    file(WRITE "${root}/upstream/c53007a.ada"
        "with Report; use Report;\nprocedure C53007A is\nbegin\n    Test (\"C53007A\", \"Harness self-test\");\n    ${body}\nend C53007A;\n")
    execute_process(
        COMMAND "${CMAKE_COMMAND}" "-DADA=${ADA}" -DNAME=c53007a
            "-DROOT=${root}" "-DWORKDIR=${root}/work"
            "-DREJECTION=${rejection}" -P "${ROOT}/RunAcats.cmake"
        TIMEOUT 55
        RESULT_VARIABLE status OUTPUT_VARIABLE output ERROR_VARIABLE errors
    )
    string(FIND "${errors}" "${expected}" found)
    if(NOT status STREQUAL "1" OR found EQUAL -1)
        message(FATAL_ERROR "Harness incorrectly graded ${scenario} (${status}):\n${output}${errors}")
    endif()
endforeach()
