package body Ada.Text_IO.Fixed_IO is
    use Ada.Text_IO;

    -- Imported Num parameters and results carry the scaled integer directly.
    -- Small is an exact power of two, so only its exponent crosses as a float.
    function Scale (Small : Long_Float) return Integer;
    pragma Import (C, Scale, "__ada_fixed_scale");
    Bits : constant Integer := Scale (Long_Float (Num'Small));

    procedure Write (File : File_Type; Item : Num; Bits, Fore, Aft, Exp : Integer);
    pragma Import (C, Write, "__ada_fixed_put");
    procedure Write (To : out String; Item : Num; Bits, Aft, Exp : Integer);
    pragma Import (C, Write, "__ada_fixed_put_string");
    function Read (File : File_Type; Bits : Integer; Low, High : Num; Width : Integer) return Num;
    pragma Import (C, Read, "__ada_fixed_get");
    function Read (From : String; Bits : Integer; Low, High : Num; Last : out Natural) return Num;
    pragma Import (C, Read, "__ada_fixed_get_string");

    procedure Get (File : in File_Type; Item : out Num; Width : in Field := 0) is
    begin
        Item := Read (File, Bits, Num'First, Num'Last, Width);
    end Get;
    procedure Get (Item : out Num; Width : in Field := 0) is
    begin
        Get (Current_Input, Item, Width);
    end Get;
    procedure Get (From : in String; Item : out Num; Last : out Positive) is
        Count : Natural;
    begin
        Item := Read (From, Bits, Num'First, Num'Last, Count);
        Last := (From'First - 1) + Count;
    end Get;

    procedure Put (File : in File_Type; Item : in Num;
                   Fore : in Field := Default_Fore;
                   Aft : in Field := Default_Aft;
                   Exp : in Field := Default_Exp) is
    begin
        Write (File, Item, Bits, Fore, Aft, Exp);
    end Put;
    procedure Put (Item : in Num;
                   Fore : in Field := Default_Fore;
                   Aft : in Field := Default_Aft;
                   Exp : in Field := Default_Exp) is
    begin
        Put (Current_Output, Item, Fore, Aft, Exp);
    end Put;
    procedure Put (To : out String; Item : in Num;
                   Aft : in Field := Default_Aft;
                   Exp : in Field := Default_Exp) is
    begin
        Write (To, Item, Bits, Aft, Exp);
    end Put;
end Ada.Text_IO.Fixed_IO;
