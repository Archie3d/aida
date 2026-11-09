package body Ada.Text_IO.Bounded_IO is
    procedure Put (File : File_Type; Item : Bounded.Bounded_String) is
    begin
        Text_IO.Put (File, Bounded.To_String (Item));
    end Put;
    procedure Put (Item : Bounded.Bounded_String) is
    begin
        Text_IO.Put (Bounded.To_String (Item));
    end Put;
    procedure Put_Line (File : File_Type; Item : Bounded.Bounded_String) is
    begin
        Text_IO.Put_Line (File, Bounded.To_String (Item));
    end Put_Line;
    procedure Put_Line (Item : Bounded.Bounded_String) is
    begin
        Text_IO.Put_Line (Bounded.To_String (Item));
    end Put_Line;
    function Get_Line (File : File_Type) return Bounded.Bounded_String is
    begin
        return Bounded.To_Bounded_String (Text_IO.Get_Line (File));
    end Get_Line;
    function Get_Line return Bounded.Bounded_String is
    begin
        return Bounded.To_Bounded_String (Text_IO.Get_Line);
    end Get_Line;
    procedure Get_Line (File : File_Type; Item : out Bounded.Bounded_String) is
    begin
        Item := Get_Line (File);
    end Get_Line;
    procedure Get_Line (Item : out Bounded.Bounded_String) is
    begin
        Item := Get_Line;
    end Get_Line;
end Ada.Text_IO.Bounded_IO;
