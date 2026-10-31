with Ada.Text_IO;
with Categorized_Client;
with Categorized_Types;
with Categorized_Pure;
procedure Categorization is
    generic
        type Pointer is access all Integer;
    function Allocate return Pointer;
    function Allocate return Pointer is
    begin
        return new Integer'(1);
    end Allocate;
    function Try_Allocate is new Allocate (Categorized_Pure.Reference);
begin
    Categorized_Client.Guard.Touch;
    Ada.Text_IO.Put_Line (Integer'Image (Categorized_Client.Sum.Value));
    Ada.Text_IO.Put_Line (Integer'Image (Categorized_Types.Get (Categorized_Client.Items.Item)));
    Ada.Text_IO.Put_Line (Integer'Image (Categorized_Client.Length));
    Ada.Text_IO.Put_Line (Integer'Image (Categorized_Client.Data.all));
    Ada.Text_IO.Put_Line (Integer'Image (Categorized_Client.Operation.all (Categorized_Pure.Value)));
    Ada.Text_IO.Put_Line (Integer'Image (Categorized_Pure.Number.all));
    Ada.Text_IO.Put_Line (Integer'Image (Categorized_Pure.Explicit_Value.Item));
    Ada.Text_IO.Put_Line (Integer'Image (Categorized_Pure.Default_Value.Item));
    begin
        declare
            Item : Categorized_Pure.Reference := Try_Allocate;
        begin
            Ada.Text_IO.Put_Line ("unexpected allocation");
        end;
    exception
        when Storage_Error => Ada.Text_IO.Put_Line ("empty pool");
    end;
end Categorization;
