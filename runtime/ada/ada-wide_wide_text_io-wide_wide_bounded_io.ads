with Ada.Strings.Wide_Wide_Bounded;
generic
    with package Wide_Wide_Bounded is new Ada.Strings.Wide_Wide_Bounded.Generic_Bounded_Length (<>);
package Ada.Wide_Wide_Text_IO.Wide_Wide_Bounded_IO is
    procedure Put (File : File_Type; Item : Wide_Wide_Bounded.Bounded_Wide_Wide_String);
    procedure Put (Item : Wide_Wide_Bounded.Bounded_Wide_Wide_String);
    procedure Put_Line (File : File_Type; Item : Wide_Wide_Bounded.Bounded_Wide_Wide_String);
    procedure Put_Line (Item : Wide_Wide_Bounded.Bounded_Wide_Wide_String);
    function Get_Line (File : File_Type) return Wide_Wide_Bounded.Bounded_Wide_Wide_String;
    function Get_Line return Wide_Wide_Bounded.Bounded_Wide_Wide_String;
    procedure Get_Line (File : File_Type; Item : out Wide_Wide_Bounded.Bounded_Wide_Wide_String);
    procedure Get_Line (Item : out Wide_Wide_Bounded.Bounded_Wide_Wide_String);
end Ada.Wide_Wide_Text_IO.Wide_Wide_Bounded_IO;
