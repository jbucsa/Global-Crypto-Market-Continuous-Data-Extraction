# IE 497 Independent Study Project Report: Global Crypto Market Continuous Data Extraction

**Authors**: Jacob Poeschel, Cameron Marchese, Jimmy McLaughlin, Meredith Naylor

## Executive Summary
Cryptocurrency markets operate globally and generate vast amounts of real-time data, yet no free, open-source system reliably collects and stores this data in an accessible way. Our project addresses this gap by developing a continuous data pipeline that connects to multiple crypto exchanges, extracts live trading data using WebSockets, timestamps it accurately with hardware, and stores it using Amazon Web Services (AWS). 

Using C programming and AWS tools, we built a system that collects data from several exchanges simultaneously and prepares it for long-term storage and public access. Current limitations in AWS resources prevent the completion of the full pipeline (e.g., uploading to a public domain), but the foundation is established and can be extended with upgraded AWS services. The ultimate goal is to provide high-quality, real-time, and historical crypto data freely to researchers, developers, and everyday investors, promoting transparency, supporting academic research, and fostering innovation in crypto trading and analysis.

## Background
The cryptocurrency market operates globally with millions of transactions, but accessing and analyzing real-time data across multiple platforms remains challenging. Existing solutions lack hardware timestamp synchronization, historical data storage, and open-source accessibility. This project bridges these gaps by creating real-time synchronized data that is openly accessible from a server platform. 

The project develops a continuous pipeline where C code connects to multiple exchanges via WebSockets to extract real-time crypto prices and trade information. The pipeline includes failsafes against the volatility of API connections to exchanges, providing valuable data to traders in the crypto market. Additionally, we aim to make all relevant crypto data available to academic institutions to support ongoing market research. Accessible data is also critical for enabling everyday investors to manage risks and position themselves effectively in crypto markets. 

Ultimately, creating a fully open-source pipeline connecting crypto exchanges to an accessible data source is a step toward democratizing financial information, enhancing transparency in a fragmented market, and fostering innovation in trading algorithms, market analysis, and decentralized finance applications.

## Introduction
This project has three primary goals:
1. Collect live cryptocurrency data from multiple exchanges using precise hardware timing.
2. Store this data in a scalable system using Amazon Web Services (AWS).
3. Make the data freely available for download and viewing on a website.

The project is fully open-source, with no cost or restrictions. Unlike existing tools, it is the first to offer both real-time and historical data from multiple crypto exchanges, with fast, accurate timing, historical storage, a user-friendly website, and protocols to handle WebSocket connection failures. 

Conducted with Professor Lariviere in the IE department due to his expertise in financial market data analysis, this project explores high-frequency trading (HFT) techniques and low-latency data collection and storage methods applicable to cryptocurrency markets. It aligns with ongoing work in financial regulation, cryptocurrency exchanges, and software development for trading.

## Technical Components
The project is a continuous pipeline that uses C code to connect to multiple crypto exchanges and extract real-time pricing and trading data. The ideal pipeline involves compressing data, obtaining precise hardware timestamps, and uploading it to an AWS server in real time, where it is then made publicly available for viewing and downloading. 

Currently, our pipeline collects data in real time, formats it readably, and uploads it to an AWS location. However, due to bottlenecks in the free tier of AWS, we could not fully complete the pipeline. Performance throttling occurs after extended runtime, halting data extraction. Additionally, the data is not yet dumped to a public domain, as this step is unnecessary until a premium AWS tier is obtained to avoid throttling. Completing the pipeline requires upgrading to a higher AWS tier, connecting it to the existing C code, and periodically outputting data to a public domain. The C code is already configured for the free AWS tier, making integration with a premium tier straightforward.

The system uses WebSockets to connect to several cryptocurrency exchanges, collecting live data such as trades, prices, and order book updates. WebSockets provide a continuous stream of real-time data, unlike periodic HTTP requests, making the process faster and more efficient. The system employs the C programming language and the `libwebsockets` library to manage real-time connections and record accurate hardware-based timestamps. Threading enables simultaneous data collection from multiple exchanges.

Hardware-based timestamps offer superior accuracy compared to software-based ones by avoiding operating system delays. *Figure 1* illustrates the difference between WebSocket and HTTP connection models. A unique feature of this system is the additional timestamp recorded when data physically arrives on the machine, providing precise data receipt timing.

<figure>
  <img align="center" src="Images/00.01-WebSocketVsHTTPConnectionModels.png" width="700">
</figure>

*Figure 1: Comparison of WebSocket and HTTP Connection Models*

After collection, data is stored on AWS, a reliable cloud platform. All processing and formatting are performed in C for speed and efficiency. Historical data is saved using AWS tools like DynamoDB or RDS, while Kinesis Streams handles real-time price and data updates. For the public-facing website, AWS services like S3 and CloudFront are planned to host and distribute the data. Note that a public S3 bucket is not currently implemented but is the goal for future iterations.

<figure>
  <img align="center" src="Images/00.02-AWSCloudArchitecture.png" width="700">
</figure>

*Figure 2: AWS Cloud Architecture*

The third component is a website and visual tools for easy data access, including real-time prices and historical data. Built with JavaScript and React, the website connects to the AWS server. While data can be downloaded directly, the website simplifies access for non-technical users. Hosted using AWS S3 and CloudFront, the site aims to maintain a simple, user-friendly interface with live price updates and historical data. Future enhancements may include charts, graphs, analytics, or a news panel.

<figure>
  <img align="center" src="Images/00.03-ExamplePriceChartMarketDataOverview.png" width="700">
</figure>    

*Figure 3: Example Price Chart and Market Data Overview*

## Code Breakdown
The project adopts a hierarchical coding approach:
1. Connect to exchanges using WebSockets.
2. Subscribe to specific data types for each exchange.
3. Process incoming WebSocket data packets and output them to a file.
4. Upload the file to an AWS server.

The code repository includes a ReadMe in each major folder, detailing how to download prerequisites, run the code, and troubleshoot issues.

## Future Project Extensions
The current implementation does not fully reflect the ideal pipeline. To achieve the goal of connecting all crypto exchanges to a publicly available open-source data reservoir, the following steps are proposed:

- **Incorporate all crypto exchanges**: Expand beyond the current five major exchanges to include all exchanges across geographical regions.
- **Retrieve all data from each exchange**: Include order book information and other relevant data, generalizing data retrieval to handle exchange-specific formats uniformly.
- **Add support for hardware timestamping WebSocket data**: Modify the `libwebsockets` library to timestamp messages upon arrival at the local network interface, ensuring nanosecond-level precision and mitigating network latency issues.
- **Incorporate historical data storage through AWS**: Upgrade AWS services (e.g., S3 for storage, DynamoDB for querying) to support robust historical data storage, currently limited by the free tier’s throttling.
- **Add AWS servers for each exchange’s geographical location**: Deploy AWS servers near exchange locations to minimize latency.
- **Utilize the available data for trading**: Develop trading algorithms, machine learning models, or predictive analytics tools using the high-quality data stream.

## Conclusion
This project demonstrates the feasibility and value of an open-source, real-time, high-precision crypto data extraction pipeline. We successfully implemented a multi-exchange WebSocket data collector in C, integrated with AWS for real-time uploads and archival. Although AWS throttling limits continuous functionality, the structure and documentation enable future contributors to build upon our work.

Future steps include expanding exchange coverage, implementing hardware timestamps, establishing historical data storage, enhancing AWS integration, and leveraging the data for trading applications. The final product will be a DevOps pipeline for continuous data extraction, synchronization, compression, analysis, and storage/display via AWS services. As the first free, open-source solution for historical and real-time global crypto market data, this project enhances market transparency, supports financial regulation, and provides a foundation for trading strategies and data analysis.

<figure>
  <img align="center" src="Images/00.04-MaketDataExtractionPipeline.png" width="700">
</figure>

*Figure 4: Market Data Extraction Pipeline*



## Authors

Justin Bucsa
- https://github.com/jbucsa
- [![LinkedIn][linkedin-shield]][linkedin-url-Bucsa]

Jimmy McLaughlin
- https://github.com/jmclaughlin19
- [![LinkedIn][linkedin-shield]][linkedin-url-McLaughlin]

Cameron Marchese
- https://github.com/cam8240
- [![LinkedIn][linkedin-shield]][linkedin-url-Marchese]

Jacob Poeschel
- https://github.com/jacobop2
- [![LinkedIn][linkedin-shield]][linkedin-url-Poeschel]

Meredith Naylor
- https://github.com/mcnaylor03
- [![LinkedIn][linkedin-shield]][linkedin-url-Naylor]


[linkedin-shield]: https://img.shields.io/badge/-LinkedIn-black.svg?style=for-the-badge&logo=linkedin&colorB=555
[linkedin-url-Bucsa]: https://www.linkedin.com/in/justin-bucsa
[linkedin-url-McLaughlin]: https://www.linkedin.com/in/james-mclaughlin-/
[linkedin-url-Marchese]: https://www.linkedin.com/in/cam8240/
[linkedin-url-Poeschel]: https://www.linkedin.com/in/jacob-poeschel/
[linkedin-url-Naylor]: https://www.linkedin.com/in/meredith-naylor/
