package body Ada.Wide_Text_IO.Scanning is
    procedure Put_Field (File : File_Type; Text : Wide_String) is
        procedure Native (File : File_Type; Text : Wide_String; Width, Operation : Integer);
        pragma Import (C, Native, "__ada_wtext_write");
    begin
        Native (File, Text, Cell_Width, 2);
    end Put_Field;
    procedure Put_Field (Text : Wide_String) is
    begin
        Put_Field (Current_Output, Text);
    end Put_Field;
    function Token (File : File_Type; Width : Field; Kind : Integer) return Wide_String is
        function Native (File : File_Type; Width : Field; Kind, Cell_Width : Integer) return Wide_String;
        pragma Import (C, Native, "__ada_wtext_scan");
    begin
        return Native (File, Width, Kind, Cell_Width);
    end Token;
    function Token (Width : Field; Kind : Integer) return Wide_String is
    begin
        return Token (Current_Input, Width, Kind);
    end Token;
    function Token_Length (Text : Wide_String; Kind : Integer) return Natural is
        function Native (Text : Wide_String; Kind, Width : Integer) return Natural;
        pragma Import (C, Native, "__ada_wtext_token_length");
    begin
        return Native (Text, Kind, Cell_Width);
    end Token_Length;
    function Modular_Token (Text : Wide_String) return Natural is
    begin
        return Token_Length (Text, 0);
    end Modular_Token;
    function Get_Modular (Last : Modular_Word; Width : Field) return Modular_Word is
    begin
        return Get_Modular (Current_Input, Last, Width);
    end Get_Modular;
    function Normalized (Text : Wide_String) return Wide_String is
        Result : Wide_String := Text;
    begin
        for I in Result'Range loop
            if Result (I) = Wide_Character'Val (9) then Result (I) := ' '; end if;
        end loop;
        return Result;
    end Normalized;
end Ada.Wide_Text_IO.Scanning;
