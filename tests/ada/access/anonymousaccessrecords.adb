with Ada.Text_IO; use Ada.Text_IO;
procedure AnonymousAccessRecords is
    Global : aliased Integer := 7;
    type Holder (Ref : access Integer) is record
        Count : Integer := 3;
    end record;
    H : Holder (Global'Access);
    type Links is array (1 .. 2) of access Integer;
    A : Links := (Global'Access, Global'Access);
    type Record_Type is record
        Ref : access Integer;
        Read_Only : access constant Integer;
    end record;
    R : Record_Type := (Global'Access, Global'Access);
    function Make (P : access Integer) return Holder is
    begin
        return (P, 9);
    end Make;
    Q : Holder := Make (Global'Access);
begin
    H.Ref.all := 11;
    Put_Line (Integer'Image (H.Ref.all + A(1).all + R.Read_Only.all + Q.Ref.all));
    declare
        Local : aliased Integer := 1;
    begin
        begin
            declare
                Escaped : Holder := Make (Local'Access);
            begin
                raise Constraint_Error;
            end;
        exception
            when Program_Error => Put_Line ("caught");
        end;
    end;
end AnonymousAccessRecords;
