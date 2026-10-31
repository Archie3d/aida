with Ada.Text_IO; use Ada.Text_IO;
procedure DerivedTypes is
    Letter : Character'Base := 'Z';
    type Derived_Character is new Character;
    Derived_Letter : Derived_Character'Base := Derived_Character (Letter);
    type Color is (Red, Green, Blue);
    subtype Middle is Color range Green .. Green;
    type Shade is new Middle;
    type Other_Shade is new Color;
    C : Color := Green;
    S : Shade := Shade (C);
    O : Other_Shade := Other_Shade (S);
    Wide : Shade'Base := Blue;
    type Box is record
        Value : Integer := 17;
    end record;
    type Child is new Box;
    type Sibling is new Box;
    B : Box;
    D : Child := Child (B);
    E : Sibling := Sibling (D);
    type Shape (Round : Boolean) is record
        case Round is
            when True => Radius : Integer := 3;
            when False => Width : Integer := 4;
        end case;
    end record;
    subtype Circle is Shape (True);
    subtype Alias_Circle is Circle;
    type Disk is new Alias_Circle;
    type Disk_Child is new Disk;
    Item : Disk_Child;
    Flat : Shape (False);
    function Read (Value : Shape) return Boolean is
    begin
        return Value.Round;
    end Read;
begin
    Put_Line (Character'Image (Character (Derived_Letter)));
    Put_Line (Color'Image (Color (O)));
    Put_Line (Shade'Image (Wide));
    Put_Line (Integer'Image (E.Value));
    D.Value := 9;
    Put_Line (Integer'Image (B.Value));
    Put_Line (Integer'Image (Item.Radius));
    Put_Line (Boolean'Image (Read (Shape (Item))));
    C := Blue;
    begin
        S := Shade (C);
        Put_Line ("MISSED enumeration constraint");
    exception
        when Constraint_Error => Put_Line ("enumeration constraint");
    end;
    begin
        Put_Line (Boolean'Image (Read (Shape (Disk (Flat)))));
        Put_Line ("MISSED discriminant constraint");
    exception
        when Constraint_Error => Put_Line ("discriminant constraint");
    end;
end DerivedTypes;
