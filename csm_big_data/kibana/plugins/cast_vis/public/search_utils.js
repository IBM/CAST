export const DefaultSearchMappings = new Object({
    'allocation-id': {
        displayText : 'Allocation ID',
        indexPattern : 'cast-allocation',
        displayFields: [
            "data.allocation_id",
            "data.primary_job_id",
            "data.secondary_job_id",
            "data.begin_time",
            "data.history.end_time",
            "data.num_nodes",
            "data.queue",
            "data.job_type",
            "data.job_name",
            "data.comment",
            "data.user_name",
            "data.state",
            "data.account"
        ]

    },
    'job-id' : {
        displayText : 'Job ID',
        indexPattern : 'cast-allocation',
        displayFields: [
            "data.allocation_id",
            "data.primary_job_id",
            "data.secondary_job_id",
            "data.begin_time",
            "data.history.end_time",
            "data.num_nodes",
            "data.queue",
            "data.job_type",
            "data.job_name",
            "data.comment",
            "data.user_name",
            "data.state",
            "data.account"
        ]
    },
    'custom' : {
        displayText : 'Custom Search',
        indexPattern : '',
        displayFields: []
    }
});

export const addSearch = (searches, search) => [...searches, search];

export const removeSearch = (searches, searchIndex) => [
    ...searches.slice(0, searchIndex),
    ...searches.slice(searchIndex + 1)
];


export const newSearch = (type) => {
    // Get the default template, falling back to custom if type is not found.
    if(! _.has(DefaultSearchMappings,type))
    {
        type = 'custom';
    }
    const template = DefaultSearchMappings[type];

    // Apply the template on object creation (there might be a better way?)
    return {
        id: (new Date()).getTime().toString(),
        indexPattern: template.indexPattern,
        indexPatternId: '',
        displayFields: template.displayFields,
        label: '',
        type: type 
    };
}

export const getTitle = (searchParams, searchIndex) => {

    return `${searchParams.type}`;
}


