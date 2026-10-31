package Context_Secret is
    type Data is record
        Value : Integer := 17;
    end record;
    function Read return Integer;
end Context_Secret;
