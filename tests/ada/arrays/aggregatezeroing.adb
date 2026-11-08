procedure AggregateZeroing is
    type Byte is mod 2 ** 8;
    for Byte'Size use 8;
    type Bytes is array (Integer range <>) of Byte;

    procedure Check0 (Seed : Byte) is
        Items : Bytes (1 .. 0) := (others => Seed);
    begin
        for I in Items'Range loop
            if Items (I) /= Seed then
                raise Program_Error;
            end if;
        end loop;
    end Check0;

    procedure Check1 (Seed : Byte) is
        Items : Bytes (1 .. 1) := (others => Seed);
    begin
        for I in Items'Range loop
            if Items (I) /= Seed then
                raise Program_Error;
            end if;
        end loop;
    end Check1;

    procedure Check2 (Seed : Byte) is
        Items : Bytes (1 .. 2) := (others => Seed);
    begin
        for I in Items'Range loop
            if Items (I) /= Seed then
                raise Program_Error;
            end if;
        end loop;
    end Check2;

    procedure Check3 (Seed : Byte) is
        Items : Bytes (1 .. 3) := (others => Seed);
    begin
        for I in Items'Range loop
            if Items (I) /= Seed then
                raise Program_Error;
            end if;
        end loop;
    end Check3;

    procedure Check4 (Seed : Byte) is
        Items : Bytes (1 .. 4) := (others => Seed);
    begin
        for I in Items'Range loop
            if Items (I) /= Seed then
                raise Program_Error;
            end if;
        end loop;
    end Check4;

    procedure Check5 (Seed : Byte) is
        Items : Bytes (1 .. 5) := (others => Seed);
    begin
        for I in Items'Range loop
            if Items (I) /= Seed then
                raise Program_Error;
            end if;
        end loop;
    end Check5;

    procedure Check7 (Seed : Byte) is
        Items : Bytes (1 .. 7) := (others => Seed);
    begin
        for I in Items'Range loop
            if Items (I) /= Seed then
                raise Program_Error;
            end if;
        end loop;
    end Check7;

    procedure Check8 (Seed : Byte) is
        Items : Bytes (1 .. 8) := (others => Seed);
    begin
        for I in Items'Range loop
            if Items (I) /= Seed then
                raise Program_Error;
            end if;
        end loop;
    end Check8;

    procedure Check9 (Seed : Byte) is
        Items : Bytes (1 .. 9) := (others => Seed);
    begin
        for I in Items'Range loop
            if Items (I) /= Seed then
                raise Program_Error;
            end if;
        end loop;
    end Check9;

    procedure Check15 (Seed : Byte) is
        Items : Bytes (1 .. 15) := (others => Seed);
    begin
        for I in Items'Range loop
            if Items (I) /= Seed then
                raise Program_Error;
            end if;
        end loop;
    end Check15;

    procedure Check16 (Seed : Byte) is
        Items : Bytes (1 .. 16) := (others => Seed);
    begin
        for I in Items'Range loop
            if Items (I) /= Seed then
                raise Program_Error;
            end if;
        end loop;
    end Check16;

    procedure Check17 (Seed : Byte) is
        Items : Bytes (1 .. 17) := (others => Seed);
    begin
        for I in Items'Range loop
            if Items (I) /= Seed then
                raise Program_Error;
            end if;
        end loop;
    end Check17;

    procedure Check31 (Seed : Byte) is
        Items : Bytes (1 .. 31) := (others => Seed);
    begin
        for I in Items'Range loop
            if Items (I) /= Seed then
                raise Program_Error;
            end if;
        end loop;
    end Check31;

    procedure Check32 (Seed : Byte) is
        Items : Bytes (1 .. 32) := (others => Seed);
    begin
        for I in Items'Range loop
            if Items (I) /= Seed then
                raise Program_Error;
            end if;
        end loop;
    end Check32;

    procedure Check33 (Seed : Byte) is
        Items : Bytes (1 .. 33) := (others => Seed);
    begin
        for I in Items'Range loop
            if Items (I) /= Seed then
                raise Program_Error;
            end if;
        end loop;
    end Check33;

    procedure Check65537 (Seed : Byte) is
        Items : Bytes (1 .. 65537) := (others => Seed);
    begin
        for I in Items'Range loop
            if Items (I) /= Seed then
                raise Program_Error;
            end if;
        end loop;
    end Check65537;

    procedure Dynamic (Count : Integer; Seed : Byte) is
        Items : Bytes (1 .. Count) := (others => Seed);
    begin
        for I in Items'Range loop
            if Items (I) /= Seed then
                raise Program_Error;
            end if;
        end loop;
    end Dynamic;
begin
    Check0 (173);
    Check1 (173);
    Check2 (173);
    Check3 (173);
    Check4 (173);
    Check5 (173);
    Check7 (173);
    Check8 (173);
    Check9 (173);
    Check15 (173);
    Check16 (173);
    Check17 (173);
    Check31 (173);
    Check32 (173);
    Check33 (173);
    Check65537 (173);
    Dynamic (32, 219);
end AggregateZeroing;
