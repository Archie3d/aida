package body Master_Model is
    overriding procedure Finalize (Object : in out Guard) is
    begin
        if Object.Id /= 0 then
            Finalized (Object.Id);
        end if;
    end Finalize;
begin
    Heap.Id := Remember_Allocation (11);
end Master_Model;
