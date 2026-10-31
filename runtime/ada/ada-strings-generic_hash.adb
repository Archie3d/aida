function Ada.Strings.Generic_Hash (Key : Text_Type) return Containers.Hash_Type is
    use type Containers.Hash_Type;
    kOffset : constant Containers.Hash_Type := 2166136261;
    kPrime : constant Containers.Hash_Type := 16777619;
    Result : Containers.Hash_Type := kOffset;
begin
    -- FNV-1a over full character positions, with modular arithmetic. Neither
    -- array bounds nor UTF encodings participate in the hash.
    for I in Key'Range loop
        Result := (Result xor Containers.Hash_Type (Character_Type'Pos (Key (I)))) * kPrime;
    end loop;
    return Result;
end Ada.Strings.Generic_Hash;
