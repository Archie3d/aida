pragma Assertion_Policy (Ignore);
package AssertionIgnoreLibrary is
    generic
        with function Condition return Boolean;
    function Check return Boolean;
end AssertionIgnoreLibrary;
