pragma Assertion_Policy (Check);
package AssertionCheckLibrary is
    pragma Suppress (All_Checks);
    pragma Unsuppress (Range_Check);
    pragma Assert (True);
    generic
        with function Condition return Boolean;
    function Check return Boolean;
end AssertionCheckLibrary;
