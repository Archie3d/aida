with Ada.Text_IO; use Ada.Text_IO;
with Primitive_Audit_Model;
procedure PrimitiveAudit is
    X : Primitive_Audit_Model.Root'Class := Primitive_Audit_Model.Make_Wide;
    Y : Primitive_Audit_Model.Root'Class := Primitive_Audit_Model.Root'(Primitive_Audit_Model.Make);
    procedure Show (Item : Primitive_Audit_Model.Root'Class) is
    begin
        Put_Line (Integer'Image (Primitive_Audit_Model.Read (Item)));
        Put_Line (Integer'Image (Primitive_Audit_Model.Combine (Item)));
    end Show;
begin
    Show (X);
    Show (Y);
    declare
        function Read (Item : Primitive_Audit_Model.Root'Class) return Integer is
        begin
            return 99;
        end Read;
    begin
        Put_Line (Integer'Image (Read (X)));
        Put_Line (Integer'Image (Primitive_Audit_Model.Read (X)));
    end;
end PrimitiveAudit;
