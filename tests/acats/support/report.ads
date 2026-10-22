-- AIDA adapter for the REPORT operations used by the selected ACATS cases.
-- This is not the official ACATS support package.
package Report is
    procedure Test (Name : String; Description : String);
    procedure Failed (Description : String);
    procedure Result;
    function Ident_Str (Value : String) return String;
    function Ident_Int (Value : Integer) return Integer;
    function Ident_Bool (Value : Boolean) return Boolean;
    function Equal (Left, Right : Integer) return Boolean;
    procedure Comment (Description : String);
    procedure Not_Applicable (Description : String);
end Report;
