with Ada.Containers; use Ada.Containers;
with Ada.Containers.Vectors;
with Controlled_Result_Model; use Controlled_Result_Model;
with Ada.Text_IO;
procedure VectorLifetime is
    package Guards is new Ada.Containers.Vectors (Positive, Guard);
    use Guards;
    procedure Exercise is
        A, B : Vector;
        Item : Guard;
        Before : Integer;
    begin
        Item.Data.Value := 42;
        Append (A, Item, 3);
        Check (Objects = 4 and Resources = 1);
        Reserve_Capacity (A, 100);
        Check (Objects = 4);
        B := A;
        Check (Objects = 7 and Resources = 1);
        Delete_First (B);
        Check (Objects = 6);
        Clear (B);
        Check (Objects = 4);
        -- Failure while constructing the second inserted element must destroy
        -- the first new element without touching the original vector.
        Fail_Adjust := 2;
        begin
            Append (A, Item, 3);
            raise Program_Error;
        exception
            when Program_Error => null;
        end;
        Check (Objects = 4 and Length (A) = 3 and Resources = 1);
        -- Failure part way through Vector.Adjust must release the partial copy.
        Fail_Adjust := 2;
        begin
            B := A;
            raise Program_Error;
        exception
            when Program_Error => null;
        end;
        Check (Objects = 4 and Length (A) = 3 and Resources = 1);
        B := A;
        Check (Objects = 7);
        Before := Objects;
        Set_Length (B, 5);
        Check (Objects = Before + 2 and Resources = 3);
        Set_Length (B, 1);
        Check (Objects = 5 and Resources = 1);
        begin
            declare
                Copy : Vector := A;
            begin
                Check (Objects = 8);
                raise Constraint_Error;
            end;
        exception
            when Constraint_Error => null;
        end;
        Check (Objects = 5);
        Fail_Finalize := True;
        begin
            Clear (A);
            raise Constraint_Error;
        exception
            when Program_Error => null;
        end;
        Check (Objects = 2 and Length (A) = 0 and Resources = 1);
    end Exercise;
begin
    Exercise;
    Check (Objects = 0 and Resources = 0);
    Ada.Text_IO.Put_Line ("vector lifetime passed");
end VectorLifetime;
