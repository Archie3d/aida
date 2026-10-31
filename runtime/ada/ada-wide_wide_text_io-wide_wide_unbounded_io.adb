package body Ada.Wide_Wide_Text_IO.Wide_Wide_Unbounded_IO is
    procedure Put (File : File_Type; Item : Strings.Wide_Wide_Unbounded.Unbounded_Wide_Wide_String) is
    begin
        Wide_Wide_Text_IO.Put (File, Strings.Wide_Wide_Unbounded.To_Wide_Wide_String (Item));
    end Put;
    procedure Put (Item : Strings.Wide_Wide_Unbounded.Unbounded_Wide_Wide_String) is
    begin
        Wide_Wide_Text_IO.Put (Strings.Wide_Wide_Unbounded.To_Wide_Wide_String (Item));
    end Put;
    procedure Put_Line (File : File_Type; Item : Strings.Wide_Wide_Unbounded.Unbounded_Wide_Wide_String) is
    begin
        Wide_Wide_Text_IO.Put_Line (File, Strings.Wide_Wide_Unbounded.To_Wide_Wide_String (Item));
    end Put_Line;
    procedure Put_Line (Item : Strings.Wide_Wide_Unbounded.Unbounded_Wide_Wide_String) is
    begin
        Wide_Wide_Text_IO.Put_Line (Strings.Wide_Wide_Unbounded.To_Wide_Wide_String (Item));
    end Put_Line;
    function Get_Line (File : File_Type) return Strings.Wide_Wide_Unbounded.Unbounded_Wide_Wide_String is
    begin
        return Strings.Wide_Wide_Unbounded.To_Unbounded_Wide_Wide_String (Wide_Wide_Text_IO.Get_Line (File));
    end Get_Line;
    function Get_Line return Strings.Wide_Wide_Unbounded.Unbounded_Wide_Wide_String is
    begin
        return Strings.Wide_Wide_Unbounded.To_Unbounded_Wide_Wide_String (Wide_Wide_Text_IO.Get_Line);
    end Get_Line;
    procedure Get_Line (File : File_Type; Item : out Strings.Wide_Wide_Unbounded.Unbounded_Wide_Wide_String) is
    begin
        Item := Get_Line (File);
    end Get_Line;
    procedure Get_Line (Item : out Strings.Wide_Wide_Unbounded.Unbounded_Wide_Wide_String) is
    begin
        Item := Get_Line;
    end Get_Line;
end Ada.Wide_Wide_Text_IO.Wide_Wide_Unbounded_IO;
