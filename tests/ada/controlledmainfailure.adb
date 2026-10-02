with Controlled_Globals;
with Controlled_Shutdown_Model; use Controlled_Shutdown_Model;
procedure ControlledMainFailure is
    Local : Guard;
begin
    raise Constraint_Error with "main failed";
end ControlledMainFailure;
