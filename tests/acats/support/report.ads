-- AIDA adapter for the REPORT operations used by the selected ACATS cases.
-- This is not the official ACATS support package.
package Report is
    procedure Test (Name : String; Description : String);
    procedure Failed (Description : String);
    procedure Result;
    function Ident_Str (Value : String) return String;
    function Ident_Char (Value : Character) return Character;
    function Ident_Wide_Char (Value : Wide_Character) return Wide_Character;
    function Ident_Int (Value : Integer) return Integer;
    function Ident_Bool (Value : Boolean) return Boolean;
    function Equal (Left, Right : Integer) return Boolean;
    subtype File_Num is Integer range 1 .. 5;
    function Legal_File_Name (X : File_Num := 1; Nam : String := "") return String;
    procedure Comment (Description : String);
    procedure Not_Applicable (Description : String);
end Report;
