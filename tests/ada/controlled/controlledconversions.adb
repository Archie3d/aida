with Controlled_Result_Model; use Controlled_Result_Model;
with Ada.Text_IO; use Ada.Text_IO;
procedure ControlledConversions is
    type Other is array (Integer range <>) of Guard;
    type Other_Matrix is array (Integer range <>, Integer range <>) of Guard;
    subtype Small_Index is Integer range 1 .. 2;
    type Small is array (Small_Index range <>) of Guard;
    function Bound return Integer is
    begin
        return 10;
    end Bound;
    procedure Borrow (Value : Other; Expected : Integer) is
    begin
        Check (Objects = Expected and Value'First = 3 and Value'Last = 5);
    end Borrow;
begin
    declare
        Source : Guards (3 .. 5);
    begin
        for I in Source'Range loop
            Source (I).Data.Value := I;
        end loop;
        Source (3).Data.Value := 42;
        declare
            Target : Other := Other (Source);
        begin
            Check (Objects = 6 and Resources = 3);
            Check (Target'First = 3 and Target (3).Data.Value = 42);
            Target := Other (Source);
            Borrow (Other (Source), 9);
            Check (Objects = 6 and Resources = 3);
        end;
        Check (Objects = 3 and Resources = 3);
        declare
            subtype Shifted is Other (Bound .. Bound + 2);
            Target : Shifted := Shifted (Source);
        begin
            Check (Target'First = 10 and Target'Last = 12 and Target (10).Data.Value = 42);
            Check (Objects = 6 and Resources = 3);
        end;
        begin
            Fail_Adjust := 2;
            Borrow (Other (Source), 6);
            raise Constraint_Error;
        exception
            when Program_Error => Check (Objects = 3 and Resources = 3);
        end;
        begin
            declare
                Bad : Small := Small (Source);
            begin
                raise Program_Error;
            end;
        exception
            when Constraint_Error => Check (Objects = 3 and Resources = 3);
        end;
        begin
            declare
                subtype Wrong_Length is Other (1 .. 2);
                Bad : Wrong_Length := Wrong_Length (Source);
            begin
                raise Program_Error;
            end;
        exception
            when Constraint_Error => Check (Objects = 3 and Resources = 3);
        end;
        Source (4 .. 5) := Guards (Other (Source (3 .. 4)));
        Check (Source (4).Data.Value = 42 and Source (5).Data.Value = 4);
        Check (Objects = 3 and Resources = 2);
    end;
    Check (Objects = 0 and Resources = 0);
    declare
        Source : Matrix (2 .. 3, 4 .. 6);
        Target : Other_Matrix := Other_Matrix (Source);
    begin
        Check (Objects = 12 and Resources = 6);
        Check (Target'First (1) = 2 and Target'First (2) = 4 and Target'Last (2) = 6);
        Target := Other_Matrix (Source);
        Check (Objects = 12 and Resources = 6);
    end;
    declare
        Empty : Guards (4 .. 3);
        Converted : Small := Small (Empty);
    begin
        Check (Converted'Length = 0 and Objects = 0 and Resources = 0);
    end;
    Put_Line ("controlled conversions passed");
end ControlledConversions;
