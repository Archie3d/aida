pragma Assertion_Policy (Check);
package body AssertionCheckLibrary is
    function Check return Boolean is
    begin
        pragma Assert (Condition);
        return True;
    end Check;
end AssertionCheckLibrary;
