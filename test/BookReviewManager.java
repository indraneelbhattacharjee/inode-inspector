package src;
import java.sql.*;
import java.util.Scanner;

public class BookReviewManager {
    private static final String URL = "jdbc:oracle:thin:@sabzevi2.homeip.net:1521:orcl";
    private static final String USER = "csus";
    private static final String PASS = "student";

    public static void main(String[] args) {
        // Load Oracle JDBC Driver
        try {
            Class.forName("oracle.jdbc.driver.OracleDriver");
        } catch (ClassNotFoundException e) {
            System.out.println("Oracle JDBC driver not found.");
            e.printStackTrace();
            return;
        }

        // Establish connection and manage resources
        try (Connection conn = DriverManager.getConnection(URL, USER, PASS)) {
            if (conn != null) {
                dropTables(conn); // Drop tables if they exist to start fresh on each run
                createTables(conn);
                Scanner scanner = new Scanner(System.in);
                boolean running = true;

                while (running) {
                    System.out.println("\nMenu:");
                    System.out.println("1) Insert");
                    System.out.println("2) Delete");
                    System.out.println("3) Update");
                    System.out.println("4) View");
                    System.out.println("5) Quit");

                    System.out.print("Choose an option: ");
                    int choice = scanner.nextInt();
                    scanner.nextLine(); // Consume newline

                    switch (choice) {
                        case 1:
                            insertRecord(conn, scanner);
                            break;
                        case 2:
                            deleteRecord(conn, scanner);
                            break;
                        case 3:
                            updateRecord(conn, scanner);
                            break;
                        case 4:
                            viewRecords(conn);
                            break;
                        case 5:
                            running = false;
                            break;
                        default:
                            System.out.println("Invalid option. Please try again.");
                    }
                }
                dropTables(conn); // Clean up tables
            }
        } catch (SQLException e) {
            System.out.println("Database error: " + e.getMessage());
            e.printStackTrace();
        }
    }

    private static void createTables(Connection conn) throws SQLException {
        DatabaseMetaData dbm = conn.getMetaData();
        // Check if "Books" table is there
        try (ResultSet rs = dbm.getTables(null, null, "BOOKS", null)) {
            if (!rs.next()) {
                System.out.println("Creating Books table...");
                String sqlBooks = "CREATE TABLE Books (" +
                        "book_id NUMBER GENERATED ALWAYS AS IDENTITY PRIMARY KEY," +
                        "title VARCHAR2(255) NOT NULL," +
                        "author VARCHAR2(255) NOT NULL)";
                try (Statement stmt = conn.createStatement()) {
                    stmt.executeUpdate(sqlBooks);
                }
            }
        }
    
        // Check if "Reviews" table is there
        try (ResultSet rs = dbm.getTables(null, null, "REVIEWS", null)) {
            if (!rs.next()) {
                System.out.println("Creating Reviews table...");
                String sqlReviews = "CREATE TABLE Reviews (" +
                        "review_id NUMBER GENERATED ALWAYS AS IDENTITY PRIMARY KEY," +
                        "book_id NUMBER NOT NULL," +
                        "review_text VARCHAR2(1000) NOT NULL," +
                        "reviewer_name VARCHAR2(255) NOT NULL," +
                        "FOREIGN KEY (book_id) REFERENCES Books(book_id))";
                try (Statement stmt = conn.createStatement()) {
                    stmt.executeUpdate(sqlReviews);
                }
            }
        }
    }
    
    private static void dropTables(Connection conn) throws SQLException {
        DatabaseMetaData dbm = conn.getMetaData();
        // Drop Reviews table if it exists
        try (ResultSet rs = dbm.getTables(null, null, "REVIEWS", null)) {
            if (rs.next()) {
                System.out.println("Dropping Reviews table...");
                try (Statement stmt = conn.createStatement()) {
                    stmt.executeUpdate("DROP TABLE Reviews CASCADE CONSTRAINTS");
                }
            }
        }
    
        // Drop Books table if it exists
        try (ResultSet rs = dbm.getTables(null, null, "BOOKS", null)) {
            if (rs.next()) {
                System.out.println("Dropping Books table...");
                try (Statement stmt = conn.createStatement()) {
                    stmt.executeUpdate("DROP TABLE Books CASCADE CONSTRAINTS");
                }
            }
        }
    }
    
    private static void insertRecord(Connection conn, Scanner scanner) throws SQLException {
        System.out.print("Enter book title: ");
        String title = scanner.nextLine();
        System.out.print("Enter book author: ");
        String author = scanner.nextLine();
    
        String sqlInsertBook = "INSERT INTO Books (title, author) VALUES (?, ?)";
        try (PreparedStatement pstmt = conn.prepareStatement(sqlInsertBook, new String[] {"book_id"})) {  // Specify the column name of the generated key
            pstmt.setString(1, title);
            pstmt.setString(2, author);
            pstmt.executeUpdate();
    
            try (ResultSet rs = pstmt.getGeneratedKeys()) {
                if (rs.next()) {
                    int bookId = rs.getInt(1);  
                    
                    System.out.print("Enter review text: ");
                    String reviewText = scanner.nextLine();
                    System.out.print("Enter reviewer's name: ");
                    String reviewerName = scanner.nextLine();
    
                    String sqlInsertReview = "INSERT INTO Reviews (book_id, review_text, reviewer_name) VALUES (?, ?, ?)";
                    try (PreparedStatement pstmtReview = conn.prepareStatement(sqlInsertReview)) {
                        pstmtReview.setInt(1, bookId);
                        pstmtReview.setString(2, reviewText);
                        pstmtReview.setString(3, reviewerName);
                        pstmtReview.executeUpdate();
                    }
                } else {
                    System.out.println("Failed to retrieve generated book ID.");
                }
            }
        }
    }
    
    private static void deleteRecord(Connection conn, Scanner scanner) throws SQLException {
        System.out.print("Enter review ID to delete: ");
        int reviewId = scanner.nextInt();
        scanner.nextLine(); // Consume newline

        String sqlDeleteReview = "DELETE FROM Reviews WHERE review_id = ?";
        try (PreparedStatement pstmtReview = conn.prepareStatement(sqlDeleteReview)) {
            pstmtReview.setInt(1, reviewId);
            int affectedRows = pstmtReview.executeUpdate();
            if (affectedRows == 0) {
                System.out.println("No review found with the given ID.");
            } else {
                System.out.println("Review deleted successfully.");
            }
        }
    }

    private static void updateRecord(Connection conn, Scanner scanner) throws SQLException {
        System.out.print("Enter book ID to update: ");
        int bookId = scanner.nextInt();
        scanner.nextLine(); // Consume newline

        System.out.print("Enter new title: ");
        String newTitle = scanner.nextLine();
        System.out.print("Enter new author: ");
        String newAuthor = scanner.nextLine();

        String sqlUpdate = "UPDATE Books SET title = ?, author = ? WHERE book_id = ?";
        try (PreparedStatement pstmt = conn.prepareStatement(sqlUpdate)) {
            pstmt.setString(1, newTitle);
            pstmt.setString(2, newAuthor);
            pstmt.setInt(3, bookId);
            int affectedRows = pstmt.executeUpdate();
            if (affectedRows > 0) {
                System.out.println("Book updated successfully.");
            } else {
                System.out.println("No book found with the given ID.");
            }
        }
    }

    private static void viewRecords(Connection conn) throws SQLException {
        String sql = "SELECT b.title, b.author, r.review_text, r.reviewer_name FROM Books b JOIN Reviews r ON b.book_id = r.book_id";
        try (Statement stmt = conn.createStatement();
             ResultSet rs = stmt.executeQuery(sql)) {
            System.out.println("\nBooks and Reviews:");
            while (rs.next()) {
                System.out.println(rs.getString("title") + " by " + rs.getString("author") +
                                   " - Review by " + rs.getString("reviewer_name") + ": \"" + rs.getString("review_text") + "\"");
            }
        }
    }
}
