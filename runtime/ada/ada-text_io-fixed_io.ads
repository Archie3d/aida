with Ada.Text_IO;

generic
    type Num is delta <>;
package Ada.Text_IO.Fixed_IO is
    use Ada.Text_IO;

    Default_Fore : Field := Num'Fore;
    Default_Aft : Field := Num'Aft;
    Default_Exp : Field := 0;

    procedure Get (File : in File_Type; Item : out Num; Width : in Field := 0);
    procedure Get (Item : out Num; Width : in Field := 0);
    procedure Get (From : in String; Item : out Num; Last : out Positive);

    procedure Put (File : in File_Type; Item : in Num;
                   Fore : in Field := Default_Fore;
                   Aft : in Field := Default_Aft;
                   Exp : in Field := Default_Exp);
    procedure Put (Item : in Num;
                   Fore : in Field := Default_Fore;
                   Aft : in Field := Default_Aft;
                   Exp : in Field := Default_Exp);
    procedure Put (To : out String; Item : in Num;
                   Aft : in Field := Default_Aft;
                   Exp : in Field := Default_Exp);
end Ada.Text_IO.Fixed_IO;
