-- AIDA adapter for the REPORT operations used by the selected ACATS cases.
-- This is not the official ACATS support package.
package Report is
    procedure Test (Name : String; Description : String);
    procedure Failed (Description : String);
    procedure Result;
    function Ident_Int (Value : Integer) return Integer;
    function Ident_Bool (Value : Boolean) return Boolean;
end Report;
