with Unchecked_Model; use Unchecked_Model;
procedure UncheckedAccessIR is
    function Lift (Object : access Integer) return Integer_Link is
    begin
        return Object.all'Unchecked_Access;
    end Lift;
    Local : aliased Integer := 1;
    P : Integer_Link := Local'Unchecked_Access;
    type Local_Type is new Root with null record;
    Object : aliased Local_Type;
    W : Root_Link := Object'Unchecked_Access;
    Saved : access Integer := Local'Unchecked_Access;
begin
    P := Lift (Local'Unchecked_Access);
    if P.all /= 1 or W.Read /= 7 then raise Program_Error; end if;
end UncheckedAccessIR;
