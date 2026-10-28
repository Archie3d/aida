pragma Assertion_Policy (Ignore);
package body AssertionIgnoreLibrary is
    function Check return Boolean is
    begin
        pragma Assert (Condition);
        return True;
    end Check;
end AssertionIgnoreLibrary;
