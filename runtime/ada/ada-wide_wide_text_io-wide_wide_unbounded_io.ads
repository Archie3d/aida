with Ada.Strings.Wide_Wide_Unbounded;
package Ada.Wide_Wide_Text_IO.Wide_Wide_Unbounded_IO is
    procedure Put (File : File_Type; Item : Strings.Wide_Wide_Unbounded.Unbounded_Wide_Wide_String);
    procedure Put (Item : Strings.Wide_Wide_Unbounded.Unbounded_Wide_Wide_String);
    procedure Put_Line (File : File_Type; Item : Strings.Wide_Wide_Unbounded.Unbounded_Wide_Wide_String);
    procedure Put_Line (Item : Strings.Wide_Wide_Unbounded.Unbounded_Wide_Wide_String);
    function Get_Line (File : File_Type) return Strings.Wide_Wide_Unbounded.Unbounded_Wide_Wide_String;
    function Get_Line return Strings.Wide_Wide_Unbounded.Unbounded_Wide_Wide_String;
    procedure Get_Line (File : File_Type; Item : out Strings.Wide_Wide_Unbounded.Unbounded_Wide_Wide_String);
    procedure Get_Line (Item : out Strings.Wide_Wide_Unbounded.Unbounded_Wide_Wide_String);
end Ada.Wide_Wide_Text_IO.Wide_Wide_Unbounded_IO;
