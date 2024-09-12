    template<typename Type>
    [[nodiscard]] basic_view<get_t<storage_for_type<const Type> >>
    view() const {
        const auto cpools = std::make_tuple(  assure<std::remove_const_t<Type>>() );
        
        basic_view< get_t< storage_for_type<const Type>>;

        std::apply([&elem](const auto *...curr) { ((curr ? elem.storage(*curr) : void()), ...); }, cpools);
        return elem;
    }