with Report; use Report;
with Length_Check;
procedure LengthCheckStorage is
    type Small is (Zero, One, Two);
    for Small'Size use 3;
    type Mapped is (Low, Middle, High);
    for Mapped use (Low => -3, Middle => 0, High => 3);
    for Mapped'Size use 3;
    type Wide is (First, Second, Third);
    for Wide use (First => 1, Second => 256, Third => 511);
    for Wide'Size use 9;
    type Signed_Wide is (Low, High);
    for Signed_Wide use (Low => -1000, High => 1000);
    for Signed_Wide'Size use 24;
    type Byte is mod 256;
    for Byte'Size use 8;
    procedure Small_Check is new Length_Check (Small);
    procedure Mapped_Check is new Length_Check (Mapped);
    procedure Integer_Check is new Length_Check (Integer);
    procedure Wide_Check is new Length_Check (Wide);
    procedure Signed_Check is new Length_Check (Signed_Wide);
    procedure Byte_Check is new Length_Check (Byte);
    procedure Check is
    begin
        Small_Check (Zero, 3, "zero");
        Small_Check (One, 3, "one");
        Small_Check (Two, 3, "two");
        Mapped_Check (Low, 3, "negative enumeration code");
        Mapped_Check (High, 3, "positive enumeration code");
        Integer_Check (Ident_Int (-17783), 32, "negative Integer");
        Integer_Check (Ident_Int (17783), 32, "positive Integer");
        Wide_Check (Third, 9, "nine-bit enumeration");
        Signed_Check (Signed_Wide'(Low), 24, "24-bit signed enumeration");
        Byte_Check (Byte (Ident_Int (173)), 8, "modular Byte");
    end Check;
    protected Results is
        procedure Passed;
        function Count return Integer;
    private
        Good : Integer := 0;
    end Results;
    protected body Results is
        procedure Passed is
        begin
            Good := Good + 1;
        end Passed;
        function Count return Integer is
        begin
            return Good;
        end Count;
    end Results;
    procedure Exercise is
        task type Worker;
        task body Worker is
        begin
            for I in 1 .. 10 loop
                Check;
            end loop;
            Results.Passed;
        end Worker;
        Workers : array (1 .. 4) of Worker;
    begin
        null;
    end Exercise;
begin
    Test ("LENGTHCHECKSTORAGE", "unchanged official Length_Check bit round trips");
    Check;
    Exercise;
    if Results.Count /= 4 then Failed ("worker failed"); end if;
    Result;
end LengthCheckStorage;
