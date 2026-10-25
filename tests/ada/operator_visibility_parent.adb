with Operator_Visibility_Parent.Private_Child;
package body Operator_Visibility_Parent is
    procedure Verify is
    begin
        if Private_Child.Value /= 1 then raise Program_Error; end if;
    end Verify;
end Operator_Visibility_Parent;
