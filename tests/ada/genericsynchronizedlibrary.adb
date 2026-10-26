package body GenericSynchronizedLibrary is
    function Identity return Ada.Tags.Tag is
    begin
        return T'Tag;
    end Identity;
    function Invoke (X : T'Class) return Integer is
    begin
        Ping (X);
        return Read (X);
    end Invoke;
end GenericSynchronizedLibrary;
