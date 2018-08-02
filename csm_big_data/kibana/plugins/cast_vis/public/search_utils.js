
export const addSearch = (searches, search) => [...searches, search];

export const removeSearch = (searches, searchIndex) => [
    ...searches.slice(0, searchIndex),
    ...searches.slice(searchIndex + 1)
];


export const newSearch = (type) => ({
    id: (new Date()).getTime().toString(),
    indexPattern: '',
    label: '',
    type: type


});

export const getTitle = (searchParams, searchIndex) => {

    return `${searchParams.type}`;
}
