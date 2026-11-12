pragma Locking_Policy (Native_Locking);
with Ada.Text_IO;
procedure NativeLocking is
    function Active return Integer;
    pragma Import (C, Active, "__ada_test_active_priority");
    protected Object is
        function Level return Integer;
    end Object;
    protected body Object is
        function Level return Integer is
        begin
            return Active;
        end Level;
    end Object;
begin
    if Object.Level /= 15 or Active /= 15 then raise Program_Error; end if;
    Ada.Text_IO.Put_Line ("native locking OK");
end NativeLocking;
