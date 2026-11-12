package body CeilingModel is
    function Active return Integer;
    pragma Import (C, Active, "__ada_test_active_priority");
    protected body Object is
        function Level return Integer is
        begin
            return Active;
        end Level;
    end Object;
begin
    if Object.Level /= 30 then raise Program_Error; end if;
end CeilingModel;
