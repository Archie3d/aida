with Ada.Text_IO;
with Abstract_Model;
with Abstract_Renamed;
procedure AbstractSubprograms is
    -- Abstract declarations need no bodies even without a dispatching profile.
    procedure Uncallable is abstract;
    function Uncallable return Integer is abstract;
    procedure Run is
        use Abstract_Model;
        use type Abstract_Model.Root;
        Item : Leaves.Last_Leaf;
        Wide : aliased Root'Class := Item;
        procedure Check (Self : in out Root'Class) is
        begin
            Via_Alias (Self);
            if Read (Self) /= 7 or else Sum (Self) /= 10 then
                raise Program_Error;
            end if;
            -- Supply a dispatching tag to a nested abstract result call.
            Self := Make;
            if Read (Self) /= 3 then
                raise Program_Error;
            end if;
            Self := Self + Make;
            if Read (Self) /= 6 then
                raise Program_Error;
            end if;
        end Check;
    begin
        if Leaves.Sum (Item) /= 6 then
            raise Program_Error;
        end if;
        Check (Wide);
        if Probe (Wide'Access) /= 6 then
            raise Program_Error;
        end if;
    end Run;
    task type Worker;
    task body Worker is
    begin
        Run;
    exception
        when others => Ada.Text_IO.Put_Line ("abstractsubprograms worker failed");
    end Worker;
begin
    Run;
    declare
        Workers : array (1 .. 4) of Worker;
    begin
        null;
    end;
    Ada.Text_IO.Put_Line ("abstract subprograms passed");
end AbstractSubprograms;
