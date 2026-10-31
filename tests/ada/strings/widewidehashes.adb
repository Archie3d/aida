with WideHashCategories;
with Ada.Text_IO;
with Ada.Containers;
with Ada.Wide_Wide_Characters;
with Ada.Strings.Wide_Wide_Hash;
with Ada.Strings.Wide_Wide_Fixed.Wide_Wide_Hash;
with Ada.Strings.Wide_Wide_Bounded.Wide_Wide_Hash;
with Ada.Strings.Wide_Wide_Unbounded.Wide_Wide_Hash;
with Ada.Containers.Indefinite_Hashed_Maps;
procedure WideWideHashes is
    use type Ada.Containers.Hash_Type;
    package B is new Ada.Strings.Wide_Wide_Bounded.Generic_Bounded_Length (32);
    package U renames Ada.Strings.Wide_Wide_Unbounded;
    function Hash (Key : Wide_Wide_String) return Ada.Containers.Hash_Type renames Ada.Strings.Wide_Wide_Hash;
    function Bounded_Hash is new Ada.Strings.Wide_Wide_Bounded.Wide_Wide_Hash (Wide_Wide_Bounded => B);
    package Maps is new Ada.Containers.Indefinite_Hashed_Maps
        (Wide_Wide_String, Integer, Ada.Strings.Wide_Wide_Fixed.Wide_Wide_Hash, "=");
    procedure Check (Ok : Boolean) is
    begin
        if not Ok then
            raise Program_Error;
        end if;
    end Check;
    procedure Exercise is
        Key : Wide_Wide_String (Positive'Last - 2 .. Positive'Last) := "😀Ж😀";
        Empty : Wide_Wide_String (7 .. 6);
        Dictionary : Maps.Map;
        Owned : U.Unbounded_Wide_Wide_String := U.To_Unbounded_Wide_Wide_String (Key);
        Bounded : B.Bounded_Wide_Wide_String := B.To_Bounded_Wide_Wide_String (Key);
    begin
        Check (Hash (Empty) = Hash (Wide_Wide_String'("")));
        Check (Hash (Wide_Wide_String'("")) = 2166136261);
        Check (Hash (Wide_Wide_String'("hello")) = 16#4F9F2CAB#);
        Check (Hash (Key) = Hash (Wide_Wide_String'("😀Ж😀")));
        Check (Hash (Key) = WideHashCategories.Hash (Key));
        Check (Hash (Key) = Ada.Strings.Wide_Wide_Fixed.Wide_Wide_Hash (Key));
        Check (Hash (Key) = Bounded_Hash (Bounded));
        Check (Hash (Key) = Ada.Strings.Wide_Wide_Unbounded.Wide_Wide_Hash (Owned));
        Check (Hash (Wide_Wide_String'(1 => Wide_Wide_Character'Val (256))) /=
            Hash (Wide_Wide_String'(1 => Wide_Wide_Character'Val (0))));
        Check (Hash (Wide_Wide_String'(1 => Wide_Wide_Character'Last)) /= Hash (Wide_Wide_String'("a")));
        Maps.Insert (Dictionary, Key, 123);
        Check (Maps.Element (Dictionary, Wide_Wide_String'("😀Ж😀")) = 123);
    end Exercise;
    task type Worker;
    task body Worker is
    begin
        Exercise;
    end Worker;
begin
    Exercise;
    declare
        Workers : array (1 .. 4) of Worker;
    begin
        null;
    end;
    Ada.Text_IO.Put_Line ("wide hashes passed");
end WideWideHashes;
