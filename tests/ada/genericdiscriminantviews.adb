with Ada.Text_IO;
procedure GenericDiscriminantViews is
    package Model is
        type Parent (N : Integer) is tagged null record;
        type Child (Size : Integer) is new Parent (Size) with record
            D : Integer;
        end record;
        type Private_Item (Count : Integer) is private;
    private
        type Private_Item (Count : Integer) is null record;
    end Model;
    generic
        type T (D : Integer) is tagged private;
    function Tagged_Read (X : T) return Integer;
    function Tagged_Read (X : T) return Integer is
    begin
        return X.D;
    end Tagged_Read;
    generic
        type T (D : Integer) is private;
    function Private_Read (X : T) return Integer;
    function Private_Read (X : T) return Integer is
    begin
        return X.D;
    end Private_Read;
    function Read_Child is new Tagged_Read (Model.Child);
    function Read_Private is new Private_Read (Model.Private_Item);
    type Link (Target : not null access Integer) is limited null record;
    generic
        type T (Pointer : not null access Integer) is limited private;
    function Dereference (X : T) return Integer;
    function Dereference (X : T) return Integer is
    begin
        return X.Pointer.all;
    end Dereference;
    function Read_Link is new Dereference (Link);
    procedure Check is
        X : Model.Child := (Size => 6, D => 90);
        Y : Model.Private_Item (8);
        Target : aliased Integer := 42;
        Z : Link (Target'Access);
    begin
        if Read_Child (X) /= 6 or X.D /= 90 or Read_Private (Y) /= 8
            or Read_Link (Z) /= 42 then
            raise Program_Error;
        end if;
    end Check;
    task type Worker;
    task body Worker is
    begin
        Check;
    end Worker;
begin
    Check;
    declare
        Workers : array (1 .. 4) of Worker;
    begin
        null;
    end;
    Ada.Text_IO.Put_Line ("generic discriminant views passed");
end GenericDiscriminantViews;
