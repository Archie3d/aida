with Ada.Text_IO.Fixed_IO;
procedure Fixedtexterrors is
    type Fixed is delta 0.125 range -10.0 .. 10.0;
    type Other is delta 0.01 range -10.0 .. 10.0;
    package Bad_IO is new Ada.Text_IO.Fixed_IO (Integer);
    X : Other := 1.0;
    Bad_Image : String := Fixed'Image (X);
    Bad_Value : Fixed := Fixed'Value (False);
    Bad_Fore : Integer := Fixed'Fore (1);
    Bad_Aft : Integer := Fixed'Aft (1);
begin
    null;
end Fixedtexterrors;
