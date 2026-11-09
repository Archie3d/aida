with Ada.Wide_Wide_Text_IO;
package Ada.Wide_Wide_Text_IO.Scanning is
    procedure Put_Field (File : File_Type; Text : Wide_Wide_String);
    procedure Put_Field (Text : Wide_Wide_String);
    function Token (File : File_Type; Width : Field; Kind : Integer) return Wide_Wide_String;
    function Token (Width : Field; Kind : Integer) return Wide_Wide_String;
    function Token_Length (Text : Wide_Wide_String; Kind : Integer) return Natural;
    function Normalized (Text : Wide_Wide_String) return Wide_Wide_String;
    type Modular_Word is mod 2 ** 64;
    function Get_Modular (Last : Modular_Word; Width : Field) return Modular_Word;
    function Get_Modular (File : File_Type; Last : Modular_Word; Width : Field) return Modular_Word;
    pragma Import (C, Get_Modular, "__ada_text_get_modular");
    function Modular_Token (Text : Wide_Wide_String) return Natural;
end Ada.Wide_Wide_Text_IO.Scanning;
