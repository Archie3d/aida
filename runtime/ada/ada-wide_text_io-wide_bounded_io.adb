package body Ada.Wide_Text_IO.Wide_Bounded_IO is
    procedure Put (File : File_Type; Item : Wide_Bounded.Bounded_Wide_String) is
    begin
        Wide_Text_IO.Put (File, Wide_Bounded.To_Wide_String (Item));
    end Put;
    procedure Put (Item : Wide_Bounded.Bounded_Wide_String) is
    begin
        Wide_Text_IO.Put (Wide_Bounded.To_Wide_String (Item));
    end Put;
    procedure Put_Line (File : File_Type; Item : Wide_Bounded.Bounded_Wide_String) is
    begin
        Wide_Text_IO.Put_Line (File, Wide_Bounded.To_Wide_String (Item));
    end Put_Line;
    procedure Put_Line (Item : Wide_Bounded.Bounded_Wide_String) is
    begin
        Wide_Text_IO.Put_Line (Wide_Bounded.To_Wide_String (Item));
    end Put_Line;
    function Get_Line (File : File_Type) return Wide_Bounded.Bounded_Wide_String is
    begin
        return Wide_Bounded.To_Bounded_Wide_String (Wide_Text_IO.Get_Line (File));
    end Get_Line;
    function Get_Line return Wide_Bounded.Bounded_Wide_String is
    begin
        return Wide_Bounded.To_Bounded_Wide_String (Wide_Text_IO.Get_Line);
    end Get_Line;
    procedure Get_Line (File : File_Type; Item : out Wide_Bounded.Bounded_Wide_String) is
    begin
        Item := Get_Line (File);
    end Get_Line;
    procedure Get_Line (Item : out Wide_Bounded.Bounded_Wide_String) is
    begin
        Item := Get_Line;
    end Get_Line;
end Ada.Wide_Text_IO.Wide_Bounded_IO;
