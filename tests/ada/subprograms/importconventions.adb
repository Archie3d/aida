with Ada.Text_IO; use Ada.Text_IO;
with Import_Provider;

procedure ImportConventions is
    function Absolute (Value : Integer) return Integer;
    pragma Import (C, Absolute, "abs");
    function Lowercase_Absolute (Value : Integer) return Integer;
    pragma Import (c, Lowercase_Absolute, "abs");
    function Increment (Value : Integer) return Integer;
    pragma Import (Ada, Increment, "import_provider__increment");
    function Mixed_Case_Increment (Value : Integer) return Integer;
    pragma Import (aDa, Mixed_Case_Increment, "import_provider__increment");
begin
    Put_Line (Integer'Image (Absolute (-7)));
    Put_Line (Integer'Image (Lowercase_Absolute (-8)));
    Put_Line (Integer'Image (Increment (40)));
    Put_Line (Integer'Image (Mixed_Case_Increment (41)));
end ImportConventions;
