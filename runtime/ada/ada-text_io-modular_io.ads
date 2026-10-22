with Ada.Text_IO;
with Ada.Text_IO.Scanning;

generic
    type Num is mod <>;

package Ada.Text_IO.Modular_IO is

    -- An instance is analysed where it is written rather than where it was
    -- declared, so what it takes from its parent it names.
    use Ada.Text_IO;

    Default_Width : Field := Num'Width;
    Default_Base  : Number_Base := 10;

    procedure Put (Item  : in Num;
                   Width : in Field := Default_Width;
                   Base  : in Number_Base := Default_Base);

    procedure Put (File  : in File_Type;
                   Item  : in Num;
                   Width : in Field := Default_Width;
                   Base  : in Number_Base := Default_Base);

    procedure Get (Item : out Num; Width : in Field := 0);

    procedure Get (File : in File_Type; Item : out Num; Width : in Field := 0);

    procedure Put (To : out String; Item : in Num; Base : in Number_Base := Default_Base);
    procedure Get (From : in String; Item : out Num; Last : out Positive);

end Ada.Text_IO.Modular_IO;
