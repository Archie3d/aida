with Ada.Containers.Vectors;
with Ada.Text_IO;
procedure ContainerDiscriminants is
    type Item (K : Boolean := False) is record
        case K is
            when False => A : Integer := 0;
            when True => B : Integer := 0;
        end case;
    end record;
    package V is new Ada.Containers.Vectors (Positive, Item);
    X : V.Vector;
    procedure Change (Element : in out Item) is
    begin
        Element := (K => True, B => 7);
    end;
begin
    V.Append (X, Item'(K => False, A => 1));
    V.Update_Element (X, 1, Change'Access);
    if not V.Element (X, 1).K then raise Program_Error; end if;
    Ada.Text_IO.Put_Line ("discriminants passed");
end ContainerDiscriminants;
