package body Ada.Text_IO.Unbounded_IO is
    procedure Put (File : File_Type; Item : Strings.Unbounded.Unbounded_String) is
    begin
        Text_IO.Put (File, Strings.Unbounded.To_String (Item));
    end Put;
    procedure Put (Item : Strings.Unbounded.Unbounded_String) is
    begin
        Text_IO.Put (Strings.Unbounded.To_String (Item));
    end Put;
    procedure Put_Line (File : File_Type; Item : Strings.Unbounded.Unbounded_String) is
    begin
        Text_IO.Put_Line (File, Strings.Unbounded.To_String (Item));
    end Put_Line;
    procedure Put_Line (Item : Strings.Unbounded.Unbounded_String) is
    begin
        Text_IO.Put_Line (Strings.Unbounded.To_String (Item));
    end Put_Line;
    function Get_Line (File : File_Type) return Strings.Unbounded.Unbounded_String is
    begin
        return Strings.Unbounded.To_Unbounded_String (Text_IO.Get_Line (File));
    end Get_Line;
    function Get_Line return Strings.Unbounded.Unbounded_String is
    begin
        return Strings.Unbounded.To_Unbounded_String (Text_IO.Get_Line);
    end Get_Line;
    procedure Get_Line (File : File_Type; Item : out Strings.Unbounded.Unbounded_String) is
    begin
        Item := Get_Line (File);
    end Get_Line;
    procedure Get_Line (Item : out Strings.Unbounded.Unbounded_String) is
    begin
        Item := Get_Line;
    end Get_Line;
end Ada.Text_IO.Unbounded_IO;
