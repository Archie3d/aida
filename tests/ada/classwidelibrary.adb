with Ada.Text_IO; use Ada.Text_IO;
with Dispatch_Model; use Dispatch_Model;
procedure ClasswideLibrary is
    A : Child;
    B : Leaf;
    procedure Show (Item : Root'Class) is
    begin
        Put_Line (Integer'Image (Value (Item)));
    end Show;
    procedure Extension (Item : Child'Class) is
    begin
        Put_Line (Integer'Image (Value (Item)));
        Put_Line (Integer'Image (Extra (Item)));
    end Extension;
begin
    Show (A);
    Show (B);
    Extension (B);
end ClasswideLibrary;
