package body Pragma_Import_Provider is
    procedure Fail (Text : String) is
    begin
        if Text'First = 3 and Text = "fail" then
            raise Constraint_Error;
        end if;
    end Fail;
    function Get return Callback is
    begin
        return Foreign_Fail'Access;
    end Get;
end Pragma_Import_Provider;
