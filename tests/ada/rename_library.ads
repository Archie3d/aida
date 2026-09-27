with Ada.Text_IO;
package Rename_Library is
    procedure Print (Text : String) renames Ada.Text_IO.Put_Line;
    function Original (X : Integer) return Integer;
    function Alias (Value : Integer := 3) return Integer renames Original;
    function Completed (Value : Integer := 4) return Integer;
    function Image (Value : Integer) return String renames Integer'Image;
    type Callback is access function (X : Integer) return Integer;
    F : Callback := Original'Access;
    function Saved (Value : Integer) return Integer renames F.all;
end Rename_Library;
