procedure DerivedCategoryErrors is
    type Flag is new Boolean;
    F : Flag := True;
    B : Boolean := True;
    Bad_Flag : Flag := B;
    Bad_Boolean : Boolean := F;
    Mixed : Flag := F and B;
    Short : Flag := F and then B;
    type Letter is new Character;
    C : Character := 'a';
    L : Letter := C;
    type Text is array (Positive range <>) of Letter;
    S : Text := "ok";
    Mixed_Text : Text := S & C;
    type Wide_Letter is new Character;
    for Wide_Letter'Size use 32;
    type Wide_Text is array (Positive range <>) of Wide_Letter;
    Wide : Wide_Text := "no";
    subtype Upper is Letter range 'A' .. 'Z';
    type Capitals is array (Positive range <>) of Upper;
    Bad_Word : Capitals := "no";
    package Hidden is
        type Switch is private;
        type Glyph is private;
    private
        type Switch is new Boolean;
        type Glyph is new Character;
    end Hidden;
    H : Hidden.Switch;
    G : Hidden.Glyph := 'x';
begin
    if H then
        null;
    end if;
end DerivedCategoryErrors;
