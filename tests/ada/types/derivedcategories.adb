with Ada.Text_IO; use Ada.Text_IO;
procedure DerivedCategories is
    type Flag is new Boolean;
    type Further is new Flag;
    A : Flag := True;
    B : Flag := False;
    C : Further := True;
    Calls : Integer := 0;
    function Touch return Flag is
    begin
        Calls := Calls + 1;
        return True;
    end Touch;
    function Choose return Flag is
    begin
        return A;
    end Choose;
    function Choose return Integer is
    begin
        return 0;
    end Choose;
    type Letter is new Character;
    type More_Letter is new Letter;
    L : Letter := 'Q';
    M : More_Letter := 'Z';
    subtype Upper is Letter range 'A' .. 'Z';
    type Capital is new Upper;
    type Capitals is array (Positive range <>) of Capital;
    Word : Capitals := "OK";
    Cap : Capital := 'A';
    type Text is array (Positive range <>) of Letter;
    S : Text := "Ada";
    T : Text := S & '!';
begin
    if A then
        Put_Line ("derived condition");
    end if;
    if Choose then
        Put_Line ("overloaded condition");
    end if;
    A := (A and not B) xor B;
    if A or else Touch then
        Put_Line ("short circuit or");
    end if;
    B := B and then Touch;
    Put_Line (Integer'Image (Calls));
    while A loop
        A := False;
    end loop;
    loop
        exit when C;
    end loop;
    Put_Line (Boolean'Image (Boolean (A)));
    Put_Line (Character'Image (Character (L)));
    Put_Line (Character'Image (Character (M)));
    Put_Line (Integer'Image (T'Length));
    Put_Line (Character'Image (Character (T (4))));
    Put_Line (Character'Image (Character (Word (2))));
    M := 'z';
    begin
        Cap := Capital (M);
        Put_Line ("MISSED character constraint");
    exception
        when Constraint_Error => Put_Line ("character constraint");
    end;
end DerivedCategories;
